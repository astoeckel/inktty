/*
 *  inktty -- Terminal emulator optimized for epaper displays
 *  Copyright (C) 2018  Andreas Stöckel
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>
#ifdef HAS_SDL

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>

#include <iostream>

#include <sys/eventfd.h>
#include <unistd.h>

#include <SDL.h>

#include <inktty/backends/sdl.hpp>
#include <inktty/gfx/epaper_emulation.hpp>

#include <time.h>

namespace inktty {
/******************************************************************************
 * Class SDLBackend::Impl                                                     *
 ******************************************************************************/

class SDLBackend::Impl {
private:
	uint32_t m_sdl_lock_event;
	uint32_t m_sdl_unlock_event;

	std::queue<SDL_Event> m_event_queue;
	int m_event_fd;

	int m_width;
	int m_height;

	std::atomic_bool m_locked;
	RGBA *m_pixels;
	int m_pitch;
	ColorLayout m_layout;

	SDL_Window *m_wnd;
	SDL_Surface *m_surf;

	std::atomic_bool m_done;
	std::atomic_bool m_initialised;
	std::mutex m_gui_mutex;
	std::thread m_gui_thread;

	std::condition_variable m_gui_cond_var;

	int m_shift;
	int m_ctrl;
	int m_alt;

	const char *m_init_err;

	bool m_epaper_emulation;

	/**
	 * This function runs on a separate thread and is the only function inside
	 * the SDLBackend::Impl class that directly interacts with SDL.
	 */
	static void sdl_main_thread(SDLBackend::Impl *self) {
		// Initialise SDL
		SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "1");
		SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
		SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
		SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
		SDL_setenv("SDL_VIDEODRIVER", "x11",
		           true);  // Prevent SDL from using the fbcon
		SDL_Init(SDL_INIT_VIDEO);

		// Create a window with the given size and abort if this fails
		self->m_wnd = SDL_CreateWindow("inktty", SDL_WINDOWPOS_UNDEFINED,
		                               SDL_WINDOWPOS_UNDEFINED, self->m_width,
		                               self->m_height, SDL_WINDOW_RESIZABLE);

		// Propagate initialisation errors to the main thread
		if (!self->m_wnd) {
			self->m_initialised = true;
			self->m_init_err = SDL_GetError();
			self->m_gui_cond_var.notify_one();
			return;
		}

		// Register the lock and unlock event
		self->m_sdl_lock_event = SDL_RegisterEvents(1);
		self->m_sdl_unlock_event = SDL_RegisterEvents(1);

		// Set the surface color layout
		self->m_layout.bpp = 32U;
		self->m_layout.rr = 0U;
		self->m_layout.rl = 0U;
		self->m_layout.gr = 0U;
		self->m_layout.gl = 8U;
		self->m_layout.br = 0U;
		self->m_layout.bl = 16U;
		self->m_layout.ar = 0U;
		self->m_layout.al = 24U;

		// Initialisation done; notify the main thread
		self->m_initialised = true;
		self->m_gui_cond_var.notify_one();

		SDL_Event event;
		while (!self->m_done) {
			if (SDL_WaitEventTimeout(&event, 100)) {
				if (event.type == self->m_sdl_lock_event) {
					// Fetch the screen width and height
					SDL_GetWindowSize(self->m_wnd, &self->m_width,
					                  &self->m_height);

					// Update the surface size in case it changed
					if (!self->m_surf || (self->m_width != self->m_surf->w ||
					                      self->m_height != self->m_surf->h)) {
						// Fetch the window surface
						self->m_surf = SDL_GetWindowSurface(self->m_wnd);
					}

					// Lock the source surface
					if (self->m_surf) {
						SDL_LockSurface(self->m_surf);
						self->m_pixels = (RGBA *)self->m_surf->pixels;
						self->m_pitch = self->m_surf->pitch;
					} else {
						self->m_pixels = nullptr;
						self->m_pitch = 0;
					}

					// Wake up the main thread
					self->m_locked = true;
					self->m_gui_cond_var.notify_one();
				} else if (event.type == self->m_sdl_unlock_event) {
					if (self->m_surf) {
						// Unlock the surface
						SDL_UnlockSurface(self->m_surf);

						// Wake up the main thread
						self->m_locked = false;
						self->m_gui_cond_var.notify_one();

						// Blit to the window
						SDL_UpdateWindowSurface(self->m_wnd);

						// Invalidate the pixel pointers
						self->m_pixels = nullptr;
						self->m_pitch = 0;
					}
				} else {
					// Forward the event to the main thread, notify the main
					// thread using the event_fd
					self->m_event_queue.push(event);
					uint64_t buf = 1;
					write(self->m_event_fd, &buf, 8);
				}
			}
		}

		// Destroy the window
		SDL_DestroyWindow(self->m_wnd);

		// Finalise SDL
		SDL_Quit();
	}

	bool sdl_handle_key_event(const SDL_KeyboardEvent &e,
	                          Event::Keyboard &keybd, bool down) {
		// Update the state of the shift/control/alt keys
		auto adj = [down](int &x) {
			x += down ? 1 : -1;
			x = std::max(0, std::min(2, x));
		};
		switch (e.keysym.sym) {
			case SDLK_LSHIFT:
			case SDLK_RSHIFT:
				adj(m_shift);
				break;
			case SDLK_LCTRL:
			case SDLK_RCTRL:
				adj(m_ctrl);
				break;
			case SDLK_LALT:
				//			case SDLK_RALT: // Do not handle AltGr
				adj(m_alt);
				break;
		}
		if (!down) {
			return false;  // Only handle key presses
		}

		// Initialise the keyboard event
		keybd.key = Event::Key::NONE;  // Not yet known
		keybd.unichar = 0;             // Not yet known
		keybd.shift = !!m_shift;
		keybd.alt = !!m_alt;
		keybd.ctrl = !!m_ctrl;

		switch (e.keysym.sym) {
			case SDLK_RETURN:
			case SDLK_RETURN2:
				keybd.key = Event::Key::ENTER;
				return true;
			case SDLK_TAB:
				keybd.key = Event::Key::TAB;
				return true;
			case SDLK_BACKSPACE:
				keybd.key = Event::Key::BACKSPACE;
				return true;
			case SDLK_ESCAPE:
				keybd.key = Event::Key::ESCAPE;
				return true;
			case SDLK_UP:
				keybd.key = Event::Key::UP;
				return true;
			case SDLK_DOWN:
				keybd.key = Event::Key::DOWN;
				return true;
			case SDLK_LEFT:
				keybd.key = Event::Key::LEFT;
				return true;
			case SDLK_RIGHT:
				keybd.key = Event::Key::RIGHT;
				return true;
			case SDLK_INSERT:
				keybd.key = Event::Key::INS;
				return true;
			case SDLK_DELETE:
				keybd.key = Event::Key::DEL;
				return true;
			case SDLK_HOME:
				keybd.key = Event::Key::HOME;
				return true;
			case SDLK_END:
				keybd.key = Event::Key::END;
				return true;
			case SDLK_PAGEUP:
				keybd.key = Event::Key::PAGE_UP;
				return true;
			case SDLK_PAGEDOWN:
				keybd.key = Event::Key::PAGE_DOWN;
				return true;
			case SDLK_F1:
			case SDLK_F2:
			case SDLK_F3:
			case SDLK_F4:
			case SDLK_F5:
			case SDLK_F6:
			case SDLK_F7:
			case SDLK_F8:
			case SDLK_F9:
			case SDLK_F10:
			case SDLK_F12:
				keybd.key =
				    static_cast<Event::Key>(static_cast<int>(Event::Key::F1) +
				                            (e.keysym.sym - SDLK_F1));
				return true;
			case SDLK_KP_0:
			case SDLK_KP_1:
			case SDLK_KP_2:
			case SDLK_KP_3:
			case SDLK_KP_4:
			case SDLK_KP_5:
			case SDLK_KP_6:
			case SDLK_KP_7:
			case SDLK_KP_8:
			case SDLK_KP_9:
				keybd.key =
				    static_cast<Event::Key>(static_cast<int>(Event::Key::KP_0) +
				                            (e.keysym.sym - SDLK_KP_0));
				break;
			case SDLK_KP_MULTIPLY:
				keybd.key = Event::Key::KP_MULT;
				break;
			case SDLK_KP_PLUS:
				keybd.key = Event::Key::KP_PLUS;
				break;
			case SDLK_KP_COMMA:
				keybd.key = Event::Key::KP_COMMA;
				break;
			case SDLK_KP_MINUS:
				keybd.key = Event::Key::KP_MINUS;
				break;
			case SDLK_KP_PERIOD:
				keybd.key = Event::Key::KP_PERIOD;
				break;
			case SDLK_KP_DIVIDE:
				keybd.key = Event::Key::KP_DIVIDE;
				break;
			case SDLK_KP_ENTER:
				keybd.key = Event::Key::KP_ENTER;
				break;
			case SDLK_KP_EQUALS:
				keybd.key = Event::Key::KP_EQUAL;
				break;
		}

		const uint32_t s = e.keysym.sym;
		const bool numlock = e.keysym.mod & KMOD_NUM;
		const bool is_ascii_ctrl = (s <= 0x1F) || (s == 0x7F);
		const bool is_unicode = s < 0x40000039;
		const bool is_numpad = (s >= 0x40000059 && s <= 0x40000063);
		const bool is_keypad = (s >= 0x40000054 && s <= 0x40000057);
		const bool is_char =
		    (is_unicode || (numlock && is_numpad) || is_keypad) &&
		    !is_ascii_ctrl;
		if (!is_char) {
			return keybd.key !=
			       Event::Key::NONE;  // Only allow the keys handled above
		}

		if (keybd.ctrl || keybd.alt) {
			keybd.unichar = e.keysym.sym;
			return true;
		}

		return false;  // Handled by TEXT_INPUT
	}

	bool sdl_handle_text_event(const SDL_TextInputEvent &e, Event::Text &txt) {
		size_t i = 0;
		for (; i < sizeof(e.text); i++) {
			char c = e.text[i];
			if (!c) {
				break;
			}
			txt.buf[i] = c;
		}
		txt.buf_len = i;
		txt.shift = m_shift;
		txt.ctrl = m_ctrl;
		txt.alt = m_alt;
		return txt.buf_len > 0;
	}

public:
	Impl(unsigned int width, unsigned int height, bool epaper_emulation)
	    : m_event_fd(eventfd(0, EFD_SEMAPHORE | EFD_CLOEXEC | EFD_NONBLOCK)),
	      m_width(width),
	      m_height(height),
	      m_locked(false),
	      m_pixels(nullptr),
	      m_wnd(nullptr),
	      m_surf(nullptr),
	      m_done(false),
	      m_initialised(false),
	      m_gui_thread(sdl_main_thread, this),
	      m_shift(0),
	      m_ctrl(0),
	      m_alt(0),
	      m_init_err(nullptr),
	      m_epaper_emulation(epaper_emulation) {

		// Wait for the GUI thread to initialise
		std::unique_lock<std::mutex> lock(m_gui_mutex);
		m_gui_cond_var.wait(lock, [this] { return !!m_initialised; });

		// Propagate initialisation errors as an exception
		if (m_init_err) {
			m_gui_thread.join();
			close(m_event_fd);
			throw std::runtime_error(m_init_err);
		}
	}

	~Impl() {
		// Wait for the GUI thread to exit
		m_done = true;
		m_gui_thread.join();

		// Close the event fd
		close(m_event_fd);
	}

	Rect lock() {
		// Signal the main thread to lock the screen surface
		SDL_Event event;
		SDL_memset(&event, 0, sizeof(event));
		event.type = m_sdl_lock_event;
		SDL_PushEvent(&event);

		// Wait for the GUI thread to acknowledge the lock
		std::unique_lock<std::mutex> lock(m_gui_mutex);
		m_gui_cond_var.wait(lock, [this] { return !!m_locked; });

		if (!m_surf) {
			return Rect();
		}
		return Rect(0, 0, m_width, m_height);
	}

	void unlock(const CommitRequest *begin, const CommitRequest *end,
	            const RGBA *buf, size_t stride) {
		// Copy the given regions to the pixel buffer
		if (!m_pixels) {
			return;
		}
		if (!m_epaper_emulation) {
			for (CommitRequest const *req = begin; req < end; req++) {
				const Rect r = req->r;
				const int w = r.width();
				for (int y = r.y0; y < r.y1; y++) {
					RGBA *tar = m_pixels + y * m_pitch / sizeof(RGBA) + r.x0;
					const RGBA *src = buf + y * stride / sizeof(RGBA) + r.x0;
					std::copy(src, src + w, tar);
				}
			}
		} else {
			for (CommitRequest const *req = begin; req < end; req++) {
				const Rect r = req->r;
				epaper_emulation::update(reinterpret_cast<uint8_t *>(m_pixels),
				                         m_pitch, m_layout, buf, stride, r.x0,
				                         r.y0, r.x1, r.y1, req->mode);
			}
		}

		// Signal the main thread to unlock the screen by sending a user event
		SDL_Event event;
		SDL_memset(&event, 0, sizeof(event));
		event.type = m_sdl_unlock_event;
		SDL_PushEvent(&event);

		// Wait until the UI has been unlocked
		std::unique_lock<std::mutex> lock(m_gui_mutex);
		m_gui_cond_var.wait(lock, [this] { return !m_locked; });
	}

	int event_fd() const { return m_event_fd; }

	EventSource::PollMode event_fd_poll_mode() const {
		return EventSource::PollIn;
	}

	bool event_get(EventSource::PollMode mode, Event &event) {
		std::lock_guard<std::mutex> lock(m_gui_mutex);

		if (!m_event_queue.empty()) {
			// Decrement the m_event_fd counter by one by calling read()
			uint64_t buf;
			read(m_event_fd, &buf, 8);

			// Fetch the last element from the queue and return
			const SDL_Event sdl_ev = m_event_queue.front();
			m_event_queue.pop();
			switch (sdl_ev.type) {
				case SDL_QUIT:
					event.type = Event::Type::QUIT;
					break;
				case SDL_KEYDOWN: {
					event.type = Event::Type::KEY_INPUT;
					return sdl_handle_key_event(sdl_ev.key, event.data.keybd,
					                            true);
				}
				case SDL_KEYUP: {
					sdl_handle_key_event(sdl_ev.key, event.data.keybd, false);
					return false;
				}
				case SDL_TEXTINPUT: {
					event.type = Event::Type::TEXT_INPUT;
					return sdl_handle_text_event(sdl_ev.text, event.data.text);
				}
			}
			return true;
		}
		return false;
	}
};

/******************************************************************************
 * Class SDLBackend                                                           *
 ******************************************************************************/

SDLBackend::SDLBackend(unsigned int width, unsigned int height,
                       bool epaper_emulation)
    : m_impl(new Impl(width, height, epaper_emulation)) {}

SDLBackend::~SDLBackend() {
	// Do nothing here, implicitly destroy the implementation
}

Rect SDLBackend::do_lock() { return m_impl->lock(); }

void SDLBackend::do_unlock(const CommitRequest *begin, const CommitRequest *end,
                           const RGBA *buf, size_t stride) {
	m_impl->unlock(begin, end, buf, stride);
}

int SDLBackend::event_fd() const { return m_impl->event_fd(); }

EventSource::PollMode SDLBackend::event_fd_poll_mode() const {
	return m_impl->event_fd_poll_mode();
}

bool SDLBackend::event_get(EventSource::PollMode mode, Event &event) {
	return m_impl->event_get(mode, event);
}

}  // namespace inktty

#endif /* HAS_SDL */
