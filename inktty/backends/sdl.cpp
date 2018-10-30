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

#include <SDL.h>

#include <inktty/backends/sdl.hpp>

namespace inktty {

/******************************************************************************
 * Class SDLBackend                                                           *
 ******************************************************************************/

SDLBackend::SDLBackend(unsigned int width, unsigned int height)
    : m_width(width), m_height(height)
{
	// Initialise SDL
	SDL_Init(SDL_INIT_VIDEO);

	// Create a window with the given size and abort if this fails
	m_wnd = SDL_CreateWindow("inktty", SDL_WINDOWPOS_UNDEFINED,
	                         SDL_WINDOWPOS_UNDEFINED, width, height,
	                         SDL_WINDOW_RESIZABLE);
	if (!m_wnd) {
		throw std::runtime_error(SDL_GetError());
	}

	// Create a surface as our back buffer
	m_surf = SDL_CreateRGBSurface(0, width, height, 24, 0xFF0000, 0x00FF00,
	                              0x0000FF, 0);
	if (!m_surf) {
		throw std::runtime_error(SDL_GetError());
	}
	m_layout.bpp = 24;
	m_layout.rr = 0;
	m_layout.rl = 16;
	m_layout.gr = 0;
	m_layout.gl = 8;
	m_layout.br = 0;
	m_layout.bl = 0;

	// Allow accessing the surface memory directly
	SDL_LockSurface(m_surf);
}

SDLBackend::~SDLBackend()
{
	// Unlock the surface
	SDL_UnlockSurface(m_surf);

	// Destroy the surface and window
	SDL_FreeSurface(m_surf);
	SDL_DestroyWindow(m_wnd);

	// Finalise SDL
	SDL_Quit();
}

uint8_t *SDLBackend::buf() const { return (uint8_t *)(m_surf->pixels); }

unsigned int SDLBackend::stride() const { return m_surf->pitch; }

bool SDLBackend::wait(int timeout)
{
	bool done = false;
	SDL_Event event;
	while ((!done) && (SDL_WaitEventTimeout(&event, timeout))) {
		switch (event.type) {
			case SDL_QUIT:
				done = true;
				break;

			default:
				break;
		}
	}
	return !done;
}

void SDLBackend::commit(int x, int y, int w, int h, CommitMode mode)
{
	/* Construct the source and target rectangle */
	SDL_Rect r{x, y, w, h};

	/* Unlock the source surface */
	SDL_UnlockSurface(m_surf);

	/* Blit to the window */
	SDL_Surface *screen = SDL_GetWindowSurface(m_wnd);
	SDL_BlitSurface(m_surf, &r, screen, &r);
	SDL_UpdateWindowSurface(m_wnd);

	/* Lock the surface */
	SDL_LockSurface(m_surf);
}

}  // namespace inktty
