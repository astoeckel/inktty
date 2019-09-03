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

#ifndef INKTTY_GFX_MATRIX_RENDERER_HPP
#define INKTTY_GFX_MATRIX_RENDERER_HPP

#include <memory>

#include <inktty/config/configuration.hpp>
#include <inktty/gfx/display.hpp>
#include <inktty/gfx/font.hpp>
#include <inktty/term/matrix.hpp>

namespace inktty {

/**
 * The MatrixRenderer class is used to render a terminal grid onto a display.
 */
class MatrixRenderer {
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;

public:
	MatrixRenderer(const Configuration &config, Font &font, Display &display,
	               Matrix &matrix, unsigned int font_size = 12 * 64,
	               unsigned int orientation = 0);

	~MatrixRenderer();

	/**
	 * Draws the matrix to the screen.
	 *
	 * @param redraw if true, redraws the entire screen.
	 * @param dt is the number of milliseconds that passed since the last call
	 * to "draw".
	 */
	void draw(bool redraw = false, int dt = 0);

	void set_font_size(unsigned int font_size);

	unsigned int font_size() const;

	void set_orientation(unsigned int rotation);

	unsigned int orientation() const;

};

}  // namespace inktty

#endif /* INKTTY_GFX_MATRIX_RENDERER_HPP */
