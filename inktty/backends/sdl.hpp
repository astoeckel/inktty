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

#ifndef INKTTY_BACKENDS_SDL_HPP
#define INKTTY_BACKENDS_SDL_HPP

#include <inktty/gfx/display.hpp>

/* Forward declarations */
struct SDL_Window;
struct SDL_Surface;

namespace inktty {

/**
 * Uses the SDL library to display a window and to wait for input events. Can
 * display a debug layer corresponding to the screen commits.
 */
class SDLBackend: public MemDisplay {
private:
	SDL_Window *m_wnd;
	SDL_Surface *m_surf;

	ColorLayout m_layout;
	unsigned int m_width;
	unsigned int m_height;

protected:
	const ColorLayout &layout() const override {
		return m_layout;
	}

	uint8_t *buf() const override;

	unsigned int stride() const override;

public:
	/**
	 * Creates a new SDLDisplay instance that opens a window with the specified
	 * width and height.
	 */
	SDLBackend(unsigned int width, unsigned int height);

	/**
	 * Destroys the SDLDisplay instance.
	 */
	~SDLBackend();

	/**
	 * Waits for events.
	 */
	bool wait(int timeout);

	void commit(int x, int y, int w, int h, CommitMode mode) override;

	unsigned int width() const override {
		return m_width;
	}

	unsigned int height() const override {
		return m_height;
	}
};
}

#endif /* INKTTY_BACKENDS_SDL_HPP */
