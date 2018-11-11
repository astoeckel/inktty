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

	SDL_Window *m_wnd;
	SDL_Surface *m_surf;

	std::atomic_bool m_done;
	std::atomic_bool m_initialised;
	std::mutex m_gui_mutex;
	std::thread m_gui_thread;

	std::condition_variable m_gui_cond_var;

	/**
	 * This function runs on a separate thread and is the only function inside
	 * the SDLBackend::Impl class that directly interacts with SDL.
	 */
	static void sdl_main_thread(SDLBackend::Impl *self) {
		// Initialise SDL
		SDL_Init(SDL_INIT_VIDEO);

		// Create a window with the given size and abort if this fails
		self->m_wnd = SDL_CreateWindow("inktty", SDL_WINDOWPOS_UNDEFINED,
		                               SDL_WINDOWPOS_UNDEFINED, self->m_width,
		                               self->m_height, SDL_WINDOW_RESIZABLE);
		if (!self->m_wnd) {
			throw std::runtime_error(SDL_GetError());
		}

		// Register the lock and unlock event
		self->m_sdl_lock_event = SDL_RegisterEvents(1);
		self->m_sdl_unlock_event = SDL_RegisterEvents(1);

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
						if (self->m_surf) {
							SDL_FreeSurface(self->m_surf);
						}

						// Create a surface as our back buffer
						self->m_surf = SDL_CreateRGBSurface(
						    0, self->m_width, self->m_height, 32, 0x000000FF,
						    0x0000FF00, 0x00FF0000, 0xFF000000);
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

						// Blit to the window
						SDL_Rect r{0, 0, self->m_width, self->m_height};
						SDL_Surface *screen = SDL_GetWindowSurface(self->m_wnd);
						SDL_BlitSurface(self->m_surf, &r, screen, &r);
						SDL_UpdateWindowSurface(self->m_wnd);

						// Invalidate the pixel pointers
						self->m_pixels = nullptr;
						self->m_pitch = 0;
					}

					// Wake up the main thread
					self->m_locked = false;
					self->m_gui_cond_var.notify_one();
				} else {
					// Forward the event to the main thread, notify the main
					// thread using the event_fd
					self->m_event_queue.push(event);
					uint64_t buf = 1;
					write(self->m_event_fd, &buf, 8);
				}
			}
		}

		// Destroy the surface and window
		if (self->m_surf) {
			SDL_FreeSurface(self->m_surf);
		}
		SDL_DestroyWindow(self->m_wnd);

		// Finalise SDL
		SDL_Quit();
	}

	static bool sdl_translate_keyboard_event(const SDL_KeyboardEvent &e,
	                                         Event::Keyboard &keybd) {
		keybd.scancode = e.keysym.scancode;
		keybd.shift = e.keysym.mod & KMOD_SHIFT;
		keybd.alt = e.keysym.mod & KMOD_ALT;
		keybd.ctrl = e.keysym.mod & KMOD_CTRL;
		const uint32_t s = e.keysym.sym;
		const bool is_ascii_ctrl = (s <= 0x1F) || (s == 0x7F);
		const bool is_unicode = s < 0x40000039;
		const bool is_numpad = (s >= 0x40000059 && s <= 0x40000063);
		const bool is_keypad = (s >= 0x40000054 && s <= 0x40000057);
		const bool numlock = e.keysym.mod & KMOD_NUM;
		keybd.is_char = !keybd.ctrl && !keybd.alt && (is_ascii_ctrl || is_unicode || (numlock && is_numpad) || is_keypad);
		keybd.codepoint = e.keysym.sym;
		return !keybd.is_char || is_ascii_ctrl;  // Characters are handled by TEXT_INPUT
	}

	static bool sdl_translate_text_event(const SDL_TextInputEvent &e,
	                                     Event::Text &txt) {
		size_t i = 0;
		for (; i < sizeof(e.text); i++) {
			char c = e.text[i];
			if (!c) {
				break;
			}
			txt.buf[i] = c;
		}
		txt.buf_len = i;
		return txt.buf_len > 0;
	}

public:
	Impl(unsigned int width, unsigned int height)
	    : m_event_fd(eventfd(0, EFD_SEMAPHORE | EFD_CLOEXEC | EFD_NONBLOCK)),
	      m_width(width),
	      m_height(height),
	      m_locked(false),
	      m_pixels(nullptr),
	      m_wnd(nullptr),
	      m_surf(nullptr),
	      m_done(false),
	      m_initialised(false),
	      m_gui_thread(sdl_main_thread, this) {
		// Wait for the GUI thread to initialise
		std::unique_lock<std::mutex> lock(m_gui_mutex);
		m_gui_cond_var.wait(lock, [this] { return !!m_initialised; });
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
		if (m_pixels) {
			for (CommitRequest const *req = begin; req < end; req++) {
				const Rect r = req->r;
				const int w = r.width();
				for (int y = r.y0; y < r.y1; y++) {
					RGBA *tar = m_pixels + y * m_pitch / sizeof(RGBA) + r.x0;
					const RGBA *src = buf + y * stride / sizeof(RGBA) + r.x0;
					std::copy(src, src + w, tar);
				}
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
					event.type = Event::Type::KEYBD_KEY_DOWN;
					return sdl_translate_keyboard_event(sdl_ev.key,
					                                    event.data.keybd);
				}
				case SDL_KEYUP: {
					event.type = Event::Type::KEYBD_KEY_UP;
					return sdl_translate_keyboard_event(sdl_ev.key,
					                                    event.data.keybd);
				}
				case SDL_TEXTINPUT: {
					event.type = Event::Type::TEXT_INPUT;
					return sdl_translate_text_event(sdl_ev.text,
					                                event.data.text);
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

SDLBackend::SDLBackend(unsigned int width, unsigned int height)
    : m_impl(new Impl(width, height)) {}

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
