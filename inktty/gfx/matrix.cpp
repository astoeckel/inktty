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

#include <algorithm>

#include <cstdio>

#include <inktty/gfx/matrix.hpp>

namespace inktty {

/******************************************************************************
 * Class Matrix                                                               *
 ******************************************************************************/

Matrix::Matrix(Font &font, Display &display, unsigned int font_size,
               unsigned int orientation)
    : m_font(font),
      m_display(display),
      m_palette(Palette::Default16Colours),
      m_font_size(font_size),
      m_orientation(orientation),
      m_cols(0),
      m_rows(0),
      m_x0(0),
      m_y0(0),
      m_x1(display.width()),
      m_y1(display.height()),
      m_pad_x(0),
      m_pad_y(0),
      m_cell_w(0),
      m_cell_h(0) {
	update_geometry();
}

void Matrix::reset() {
	m_cells.clear();
	update_geometry();
}

void Matrix::update_geometry() {
	/* Fetch the font geometry */
	const MonospaceFontMetrics m = m_font.metrics(m_font_size);
	m_cell_w = m.cell_width;
	m_cell_h = m.cell_height;

	/* Fetch the screen geometry */
	const unsigned int o = m_orientation;
	const size_t w = std::max(0, (o & 1) ? (m_y1 - m_y0) : (m_x1 - m_x0));
	const size_t h = std::max(0, (o & 1) ? (m_x1 - m_x0) : (m_y1 - m_y0));

	/* Compute the number of cells and the padding */
	m_cols = w / m_cell_w;
	m_rows = h / m_cell_h;
	m_pad_x = (w - m_cell_w * m_cols) / 2;
	m_pad_y = (h - m_cell_h * m_rows) / 2;

	/* Resize the cell buffer and mark all cells as dirty. Do not decrease the
	   size of the buffers to avoid information loss. */
	if (m_cells.size() < m_rows) {
		m_cells.resize(m_rows);
	}
	for (size_t i = 0; i < m_rows; i++) {
		if (m_cells[i].size() < m_cols) {
			m_cells[i].resize(m_cols);
		}
		for (size_t j = 0; j < m_cols; j++) {
			m_cells[i][j].status = Cell::Status::dirty;
		}
	}

	/* The geometry has been updated, prevent unecessary calls to this
	   function. */
	m_needs_geometry_update = false;
}

void Matrix::set_cell(size_t row, size_t col, uint32_t glyph,
                      const Style &style) {
	/* Abort if the coordinates are invalid */
	if (row >= m_rows || col >= m_cols) {
		return;
	}

	/* Update the cell in case anything actually changed */
	Cell &c = m_cells[row][col];
	if (glyph != c.glyph || style != c.style) {
		c.status = Cell::Status::dirty;
		c.glyph = glyph;
		c.style = style;
	}
}

Rect Matrix::get_coords(size_t row, size_t col) {
	/* Compute the untransformed bounding box */
	const int x0 = col * m_cell_w;
	const int x1 = x0 + m_cell_w;
	const int y0 = row * m_cell_h;
	const int y1 = y0 + m_cell_h;

	/* Return the bounding box depending on the orientation */
	switch (m_orientation) {
		default:
		case 0:
			return Rect{
			    m_x0 + m_pad_x + x0,
			    m_y0 + m_pad_y + y0,
			    m_x0 + m_pad_x + x1,
			    m_y0 + m_pad_y + y1,
			};
		case 1:
			return Rect{
			    m_x0 + m_pad_y + y0,
			    m_y1 - m_pad_x - x1,
			    m_x0 + m_pad_y + y1,
			    m_y1 - m_pad_x - x0,
			};
		case 2:
			return Rect{
			    m_x1 - m_pad_x - x1,
			    m_y1 - m_pad_y - y1,
			    m_x1 - m_pad_x - x0,
			    m_y1 - m_pad_y - y0,
			};
		case 3:
			return Rect{
			    m_x1 - m_pad_y - y1,
			    m_y0 + m_pad_x + x0,
			    m_x1 - m_pad_y - y0,
			    m_y0 + m_pad_x + x1,
			};
	}
	__builtin_unreachable();
}

void Matrix::update() {
	if (m_needs_geometry_update) {
		update_geometry();
	}
}

void Matrix::draw() {
	update();
	for (size_t i = 0; i < m_rows; i++) {
		for (size_t j = 0; j < m_cols; j++) {
			Cell &c = m_cells[i][j];
			if (c.status == Cell::Status::dirty) {
				/* Fetch foreground and background colour */
				const RGB fg = c.style.fg.rgb(m_palette);
				const RGB bg = c.style.bg.rgb(m_palette);

				/* Draw the background */
				const Rect r = get_coords(i, j);
				m_display.fill(bg, r.x0, r.y0, r.width(), r.height());

				/* Draw the glyph */
				const GlyphBitmap *g = m_font.render(c.glyph, m_font_size, false, m_orientation);
				if (g) {
					m_display.blit(g->buf(), g->stride, fg, bg, r.x0 + g->x,
							       r.y0 + g->y, g->w, g->h);
				}

				/* Mark the cell as "drawn" */
				c.status = Cell::Status::high_quality;
			}
		}
	}
}

void Matrix::set_orientation(unsigned int orientation) {
	/* There are four possible orientations */
	orientation = orientation % 4;

	/* Trigger a full update if the orientation changed */
	if (orientation != m_orientation) {
/*		m_display.fill(RGB::Black, 0, 0, m_display.width(), m_display.height());*/
		m_orientation = orientation;
		m_needs_geometry_update = true;
	}
}

}  // namespace inktty
