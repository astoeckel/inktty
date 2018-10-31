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

#include <stdexcept>

#include <sys/eventfd.h>
#include <unistd.h>

#include <inktty/backends/sdl.hpp>

namespace inktty {

/******************************************************************************
 * STATIC HELPER FUNCTIONS                                                    *
 ******************************************************************************/

/******************************************************************************
 * CLASS IMPLEMENTATIONS                                                      *
 ******************************************************************************/

/******************************************************************************
 * Class SDLBackend                                                           *
 ******************************************************************************/

SDLBackend::SDLBackend(unsigned int width, unsigned int height)
    : m_event_fd(eventfd(0, EFD_SEMAPHORE | EFD_CLOEXEC | EFD_NONBLOCK)),
      m_done(false),
      m_width(width),
      m_height(height),
      m_wnd(nullptr),
      m_surf(nullptr),
      m_gui_thread(SDLBackend::gui_thread_main, this) {}

SDLBackend::~SDLBackend() {
	// Wait for the GUI thread to exit
	m_done = true;
	m_gui_thread.join();

	// Close the event fd
	close(m_event_fd);
}

void SDLBackend::gui_thread_main(SDLBackend *self) {
	// Initialise SDL
	SDL_Init(SDL_INIT_VIDEO);

	// Create a window with the given size and abort if this fails
	self->m_wnd = SDL_CreateWindow("inktty", SDL_WINDOWPOS_UNDEFINED,
	                               SDL_WINDOWPOS_UNDEFINED, self->m_width,
	                               self->m_height, SDL_WINDOW_RESIZABLE);
	if (!self->m_wnd) {
		throw std::runtime_error(SDL_GetError());
	}

	// Create a surface as our back buffer
	self->m_surf = SDL_CreateRGBSurface(0, self->m_width, self->m_height, 24,
	                                    0xFF0000, 0x00FF00, 0x0000FF, 0);
	if (!self->m_surf) {
		throw std::runtime_error(SDL_GetError());
	}
	self->m_layout.bpp = 24;
	self->m_layout.rr = 0;
	self->m_layout.rl = 16;
	self->m_layout.gr = 0;
	self->m_layout.gl = 8;
	self->m_layout.br = 0;
	self->m_layout.bl = 0;

	SDL_Event event;
	while (!self->m_done) {
		if (SDL_WaitEventTimeout(&event, 100)) {
			std::lock_guard<std::mutex> lock(self->m_gui_mutex);

			self->m_event_queue.push(event);

			uint64_t buf = 1;
			write(self->m_event_fd, &buf, 8);
		}
	}

	// Destroy the surface and window
	SDL_FreeSurface(self->m_surf);
	SDL_DestroyWindow(self->m_wnd);

	// Finalise SDL
	SDL_Quit();
}

uint8_t *SDLBackend::buf() const { return (uint8_t *)(m_surf->pixels); }

unsigned int SDLBackend::stride() const { return m_surf->pitch; }

void SDLBackend::commit(int x, int y, int w, int h, CommitMode mode) {
	// TODO: Move to main thread!

	// Construct the source and target rectangle
	SDL_Rect r{x, y, w, h};

	// Unlock the source surface
	SDL_UnlockSurface(m_surf);

	// Blit to the window
	SDL_Surface *screen = SDL_GetWindowSurface(m_wnd);
	SDL_BlitSurface(m_surf, &r, screen, &r);
	SDL_UpdateWindowSurface(m_wnd);

	// Lock the surface
	SDL_LockSurface(m_surf);
}

int SDLBackend::event_fd() const { return m_event_fd; }

EventSource::PollMode SDLBackend::event_fd_poll_mode() const {
	return EventSource::poll_in;
}

bool SDLBackend::event_get(EventSource::PollMode mode, Event &event) {
	std::lock_guard<std::mutex> lock(m_gui_mutex);

	if (!m_event_queue.empty()) {
		/* Decrement the m_event_fd counter by one by calling read() */
		uint64_t buf;
		read(m_event_fd, &buf, 8);

		// Fetch the last element from the queue and return
		const SDL_Event &sdl_ev = m_event_queue.front();
		switch (sdl_ev.type) {
			case SDL_QUIT:
				event.type = Event::Type::QUIT;
				break;
		}
		m_event_queue.pop();
		return true;
	}
	return false;
}

}  // namespace inktty
