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
#include <iostream>

#include <inktty/gfx/matrix.hpp>

namespace inktty {

/******************************************************************************
 * Class Matrix                                                               *
 ******************************************************************************/

Matrix::Matrix(Font &font, Display &display, unsigned int font_size,
               unsigned int orientation)
    : m_font(font),
      m_display(display),
      m_palette(Palette::Default256Colours),
      m_font_size(font_size),
      m_orientation(orientation),
      m_cols(0),
      m_rows(0),
      m_bounds(0, 0, 0, 0),
      m_pad_x(0),
      m_pad_y(0),
      m_cell_w(0),
      m_cell_h(0),
      m_needs_geometry_update(true),
      m_cursor_row(0),
      m_cursor_col(0),
      m_row_offs(0) {
	draw();
	reset();
}

void Matrix::reset() {
	m_cursor_row = 0;
	m_cursor_col = 0;
	m_row_offs = 0;
	m_cursor_visible = true;
	m_cells.clear();
	update_geometry();
}

void Matrix::update_geometry() {
	/* Fetch the font geometry */
	const MonospaceFontMetrics m = m_font.metrics(m_font_size);
	m_cell_w = m.cell_width;
	m_cell_h = m.cell_height;

	/* Fetch the bounding box */
	const int b_x0 = m_bounds.x0, b_y0 = m_bounds.y0;
	const int b_x1 = m_bounds.x1, b_y1 = m_bounds.y1;

	/* Fetch the screen geometry */
	const unsigned int o = m_orientation;
	const size_t w = std::max(0, (o & 1) ? (b_y1 - b_y0) : (b_x1 - b_x0));
	const size_t h = std::max(0, (o & 1) ? (b_x1 - b_x0) : (b_y1 - b_y0));

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
	// Compute the untransformed bounding box
	const int x0 = col * m_cell_w;
	const int x1 = x0 + m_cell_w;
	const int y0 = row * m_cell_h;
	const int y1 = y0 + m_cell_h;

	const int b_x0 = m_bounds.x0, b_y0 = m_bounds.y0;
	const int b_x1 = m_bounds.x1, b_y1 = m_bounds.y1;

	// Return the bounding box depending on the orientation
	switch (m_orientation) {
		default:
		case 0:
			return Rect{
			    b_x0 + m_pad_x + x0,
			    b_y0 + m_pad_y + y0,
			    b_x0 + m_pad_x + x1,
			    b_y0 + m_pad_y + y1,
			};
		case 1:
			return Rect{
			    b_x0 + m_pad_y + y0,
			    b_y1 - m_pad_x - x1,
			    b_x0 + m_pad_y + y1,
			    b_y1 - m_pad_x - x0,
			};
		case 2:
			return Rect{
			    b_x1 - m_pad_x - x1,
			    b_y1 - m_pad_y - y1,
			    b_x1 - m_pad_x - x0,
			    b_y1 - m_pad_y - y0,
			};
		case 3:
			return Rect{
			    b_x1 - m_pad_y - y1,
			    b_y0 + m_pad_x + x0,
			    b_x1 - m_pad_y - y0,
			    b_y0 + m_pad_x + x1,
			};
	}
	__builtin_unreachable();
}

void Matrix::update() {
	if (m_needs_geometry_update) {
		update_geometry();
	}
}

void Matrix::set_cursor_position(int row, int col) {
	// TODO: This code is ugly
	if (row != m_cursor_row || col != m_cursor_col) {
		if (m_cursor_row >= 0 && m_cursor_col >= 0 &&
		    m_cursor_row < int(m_rows) && m_cursor_col < int(m_cols)) {
			m_cells[m_cursor_row][m_cursor_col].cursor = false;
			m_cells[m_cursor_row][m_cursor_col].status = Cell::Status::dirty;
		}
		m_cursor_row = row;
		m_cursor_col = col;
		if (m_cursor_row >= 0 && m_cursor_col >= 0 &&
		    m_cursor_row < int(m_rows) && m_cursor_col < int(m_cols)) {
			m_cells[m_cursor_row][m_cursor_col].cursor = true;
			m_cells[m_cursor_row][m_cursor_col].status = Cell::Status::dirty;
		}
	}
}

void Matrix::set_cursor_visible(bool visible) {
	if (visible != m_cursor_visible) {}
}

void Matrix::draw() {
	const Rect new_bounds = m_display.lock();  // TODO update screen size
	if (new_bounds != m_bounds) {
		m_bounds = new_bounds;
		m_needs_geometry_update = true;
	}

	update();

	for (size_t i = 0; i < m_rows; i++) {
		for (size_t j = 0; j < m_cols; j++) {
			Cell &c = m_cells[i + m_row_offs][j];
			if (c.status == Cell::Status::dirty) {
				/* Fetch foreground and background colour */
				RGBA fg = c.style.fg.rgb(m_palette);
				RGBA bg = c.style.bg.rgb(m_palette);
				if (c.cursor ^ c.style.inverse) {
					std::swap(fg, bg);
				}

				/* Draw the background */
				const Rect r = get_coords(i, j);
				Rect gr = r;
				m_display.fill(Display::Layer::Background, bg, r);

				/* Draw the glyph */
				const GlyphBitmap *g =
				    m_font.render(c.glyph, m_font_size, false, m_orientation);
				if (g) {
					gr = Rect::sized(r.x0 + g->x, r.y0 + g->y, g->w, g->h);
					m_display.blit(Display::Layer::Presentation, fg, g->buf(),
					               g->stride, gr);
				}

				/* Mark the cell as "drawn" */
				c.status = Cell::Status::high_quality;

				/* Commit the cell */
				m_display.commit(r.grow(gr), Display::CommitMode::Full);
			}
		}
	}
	m_display.unlock();
}

void Matrix::set_orientation(unsigned int orientation) {
	/* There are four possible orientations */
	orientation = orientation % 4;

	/* Trigger a full update if the orientation changed */
	if (orientation != m_orientation) {
		/*		m_display.fill(RGB::Black, 0, 0, m_display.width(),
		 * m_display.height());*/
		m_orientation = orientation;
		m_needs_geometry_update = true;
	}
}

void Matrix::scroll() {
	size_t max_scroll_buffer = m_rows * 64;
	m_cells.emplace_back(m_cols, Cell{});
	for (size_t i = m_row_offs; i < m_row_offs + m_rows; i++) {
		for (size_t j = 0; j < m_cols; j++) {
			m_cells[i][j].status = Cell::Status::dirty;
		}
	}
	m_row_offs++;
	if (m_row_offs > max_scroll_buffer) {
		m_cells.erase(m_cells.begin(), m_cells.begin() + max_scroll_buffer);
		m_row_offs -= max_scroll_buffer;
	}
}

}  // namespace inktty
