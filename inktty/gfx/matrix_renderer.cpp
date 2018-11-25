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

#include <inktty/gfx/matrix_renderer.hpp>

namespace inktty {

/******************************************************************************
 * Class MatrixRenderer                                                       *
 ******************************************************************************/

MatrixRenderer::MatrixRenderer(Font &font, Display &display, Matrix &matrix,
                               unsigned int font_size, unsigned int orientation)
    : m_font(font),
      m_display(display),
      m_matrix(matrix),
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
      m_needs_geometry_update(true) {
	// Temporarily lock/unlock the display to get the screen size
	m_bounds = m_display.lock();
	m_display.unlock();

	// Compute the geometry
	update_geometry();

	for (size_t i = 0; i < 16; i++) {
		m_palette[i] = Palette::Solarized16Colours[i];
	}
}

void MatrixRenderer::update_geometry() {
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

	/* Resize the underlying matrix instance */
	m_matrix.resize(m_rows, m_cols);

	/* The geometry has been updated, prevent unecessary calls to this
	   function. */
	m_needs_geometry_update = false;
}

Rect MatrixRenderer::get_coords(size_t row, size_t col) {
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

Rect MatrixRenderer::draw_cell(size_t row, size_t col, const Matrix::Cell &cell, bool erase) {
	/* Fetch foreground and background colour */
	RGBA fg = cell.style.fg.rgb(m_palette);
	RGBA bg = cell.style.bg.rgb(m_palette);
	if (cell.style.default_fg) {
		fg = RGBA::SolarizedWhite;
	}
	if (cell.style.default_bg) {
		bg = RGBA::SolarizedBlack;
	}
	if (cell.cursor ^ cell.style.inverse) {
		std::swap(fg, bg);
	}

	// Draw the background
	const Rect r = get_coords(row, col);
	Rect gr = r;
	if (!erase) {
		m_display.fill(Display::Layer::Background, bg, r);
	}

	// Draw the glyph
	const GlyphBitmap *g =
	    m_font.render(cell.glyph, m_font_size, false, m_orientation);
	if (g) {
		gr = Rect::sized(r.x0 + g->x, r.y0 + g->y, g->w, g->h);
		m_display.blit(Display::Layer::Presentation, fg, g->buf(), g->stride,
		               gr, erase ? Display::DrawMode::Erase : Display::DrawMode::Write);
	}

	return r.grow(gr);
}

void MatrixRenderer::draw(bool redraw) {
	// Check whether the geometry needs to be updated
	const Rect new_bounds = m_display.lock();  // TODO update screen size
	if (new_bounds != m_bounds) {
		m_bounds = new_bounds;
		m_needs_geometry_update = true;
	}
	if (m_needs_geometry_update) {
		update_geometry();  // Resets m_needs_geometry_update
	}

	// Either redraw the entire screen or fetch a partial screen update
	if (redraw) {
		m_display.fill(Display::Layer::Background, RGBA::Black, m_bounds);
		m_display.fill(Display::Layer::Presentation, RGBA(0, 0, 0, 0), m_bounds);
		Matrix::CellArray cells = m_matrix.cells();
		Matrix::Cell cell_old;
		for (size_t i = 0; i < m_matrix.size().y; i++) {
			for (size_t j = 0; j < m_matrix.size().x; j++) {
				const Rect r1 = draw_cell(i, j, cell_old, true);
				const Rect r2 = draw_cell(i, j, cells[i][j], false);
				m_display.commit(r1.grow(r2), Display::CommitMode::Full);
			}
		}
	} else {
		std::vector<Matrix::CellUpdate> updates;
		m_matrix.commit(updates);
		for (size_t i = 0; i < updates.size(); i++) {
			const Matrix::CellUpdate &u = updates[i];
			const Rect r1 = draw_cell(u.pos.y - 1, u.pos.x - 1, u.old, true);
			const Rect r2 = draw_cell(u.pos.y - 1, u.pos.x - 1, u.current, false);
			m_display.commit(r1.grow(r2), Display::CommitMode::Full);
		}
	}

	m_display.unlock();
}

void MatrixRenderer::set_orientation(unsigned int orientation) {
	/* There are four possible orientations */
	orientation = orientation % 4;

	/* Trigger a full update if the orientation changed */
	if (orientation != m_orientation) {
		m_display.fill(Display::Layer::Background, RGBA::Black, m_bounds);
		m_display.fill(Display::Layer::Presentation, RGBA(0, 0, 0, 0), m_bounds);
		m_orientation = orientation;
		m_needs_geometry_update = true;
	}
}
}  // namespace inktty
