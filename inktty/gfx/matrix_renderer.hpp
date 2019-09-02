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

#include <vector>

#include <inktty/config/configuration.hpp>
#include <inktty/gfx/display.hpp>
#include <inktty/gfx/font.hpp>
#include <inktty/term/matrix.hpp>
#include <inktty/utils/color.hpp>
#include <inktty/utils/geometry.hpp>

#ifndef INKTTY_GFX_MATRIX_RENDERER_HPP
#define INKTTY_GFX_MATRIX_RENDERER_HPP

namespace inktty {

/**
 * The MatrixRenderer class is used to render a terminal grid onto a display.
 */
class MatrixRenderer {
private:
	const Configuration &m_config;

	Font &m_font;

	Display &m_display;

	Matrix &m_matrix;

	unsigned int m_font_size;

	unsigned int m_orientation;

	size_t m_cols, m_rows;

	Rect m_bounds;

	int m_pad_x, m_pad_y;

	size_t m_cell_w, m_cell_h;

	bool m_needs_geometry_update;

	void update_geometry();

	Rect get_coords(size_t row, size_t col);

	Rect draw_cell(size_t row, size_t col, const Matrix::Cell &cell,
	               bool erase, bool low_quality=true);

public:
	MatrixRenderer(const Configuration &config, Font &font, Display &display, Matrix &matrix,
	       unsigned int font_size = 12 * 64, unsigned int orientation = 0);

	/**
	 * Clears all cell content.
	 */
	void reset();

	/**
	 * Draws the matrix to the screen.
	 *
	 * @param redraw if true, redraws the entire screen.
	 * @param dt is the number of milliseconds that passed since the last call
	 * to "draw".
	 */
	void draw(bool redraw = false, int dt = 0);

	void set_font_size(unsigned int font_size);

	unsigned int font_size() const { return m_font_size; }

	void set_orientation(unsigned int rotation);

	unsigned int orientation() const { return m_orientation; }

	/**
	 * Returns the number of cells in x-direction.
	 */
	size_t cols() const { return m_matrix.size().x; }

	/**
	 * Returns the number of cells in y-direction.
	 */
	size_t rows() const { return m_matrix.size().y; }
};

}  // namespace inktty

#endif /* INKTTY_GFX_MATRIX_RENDERER_HPP */
