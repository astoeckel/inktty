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

#include <inktty/term/matrix.hpp>

namespace inktty {

/******************************************************************************
 * Class Matrix::Cell                                                         *
 ******************************************************************************/

bool Matrix::Cell::invisible() const {
	if (style.concealed) {
		return true;
	}
	if (style.strikethrough || style.underline) {
		return false;
	}
	if (!glyph || glyph == ' ') {
		return true;
	}
	return false;
}

bool Matrix::Cell::needs_update(const Cell &old) const {
	// No need to check anything if the "dirty" flag is not set
	if (!dirty) {
		return false;
	}

	// Need to update if the "cursor" flag changed
	const bool inverse = cursor ^ style.inverse;
	const bool inverse_old = old.cursor ^ old.style.inverse;
	if (inverse != inverse_old) {
		return true;
	}

	// Check whether the foreground is invisible. If it is invisible in both
	// this Cell and the old cell, we do not need to check the foreground
	const bool need_check_fg = !(invisible() && old.invisible());
	if (need_check_fg) {
		if (glyph != old.glyph) {
			return true;
		}
		if (!inverse) {
			if (style.fg != old.style.fg) {
				return true;
			}
		} else {
			if (style.bg != old.style.bg) {
				return true;
			}
		}
		if ((style.bold != old.style.bold) ||
		    (style.italic != old.style.italic) ||
		    (style.strikethrough != old.style.strikethrough) ||
		    (style.underline != old.style.underline)) {
			return true;
		}
	}

	// Check the background
	if (!inverse) {
		if (style.bg != old.style.bg) {
			return true;
		}
	} else {
		if (style.fg != old.style.fg) {
			return true;
		}
	}
	return false;
}

/******************************************************************************
 * Class Matrix                                                               *
 ******************************************************************************/

Matrix::Matrix(int rows, int cols)
    : m_pos(1, 1),
      m_pos_old(1, 1),
      m_size(cols, rows),
      m_cursor_visible(true),
      m_cursor_visible_old(false),
      m_alternative_buffer_active(false) {
	reset();
}

bool Matrix::valid(const Point &p) const {
	return (p.x >= 1) && (p.y >= 1) && (p.x <= m_size.x) && (p.y <= m_size.y);
}

void Matrix::extend_update_bounds(const Point &p) {
	m_update_bounds.x0 = std::min(m_update_bounds.x0, p.x);
	m_update_bounds.y0 = std::min(m_update_bounds.y0, p.y);
	m_update_bounds.x1 = std::max(m_update_bounds.x1, p.x);
	m_update_bounds.y1 = std::max(m_update_bounds.y1, p.y);
}

void Matrix::reset() {
	// Set the new cursor location
	m_pos = Point(1, 1);

	// Make the cursor visible
	m_cursor_visible = true;

	// Resize the vectors to the current matrix size. This deletes any content
	// that might have been left over from a previous resize().
	m_cells.resize(m_size.y);
	m_cells_alt.resize(m_size.y);
	m_cells_old.resize(m_size.y);
	for (int y = 0; y < m_size.y; y++) {
		m_cells[y].resize(m_size.x);
		m_cells_alt[y].resize(m_size.x);
		m_cells_old[y].resize(m_size.x);
	}

	// Reset all cells to their initial, empty state
	for (int y = 1; y <= m_size.y; y++) {
		for (int x = 1; x <= m_size.x; x++) {
			set(0, Style{}, Point{x, y});
		}
	}
}

void Matrix::resize(int rows, int cols) {
	// Make sure rows, cols is valid
	rows = std::max(0, rows);
	cols = std::max(0, cols);

	// Set the new size
	m_size = Point(cols, rows);

	// Add new rows to the matrix
	if ((size_t)rows > m_cells.size()) {
		m_cells.resize(rows);
		m_cells_alt.resize(rows);
		m_cells_old.resize(rows);
	}

	// Add new colums to the matrix
	for (int y = 0; y < rows; y++) {
		if ((size_t)cols > m_cells[y].size()) {
			m_cells[y].resize(cols);
			m_cells_alt[y].resize(cols);
			m_cells_old[y].resize(cols);
		}
	}

	// Limit the update rectangle to the new size
	m_update_bounds.x1 = std::min(m_update_bounds.x1, cols);
	m_update_bounds.y1 = std::min(m_update_bounds.y1, rows);
}

void Matrix::move_abs(Point pos) {
	m_pos = Rect{1, 1, m_size.x, m_size.y}.clip(pos, true);
}

void Matrix::set(uint32_t glyph, const Style &style, Point pos) {
	// Make sure "pos" is valid
	if (!valid(pos)) {
		return;
	}

	// Superficially check whether the cell content has changed; if yes, assign
	// the new cell content and mark the cell as "dirty"
	Cell &c = m_cells[pos.y - 1][pos.x - 1];
	if (glyph != c.glyph || style != c.style) {
		c.glyph = glyph;
		c.style = style;
		c.dirty = true;

		// Grow the update bounds
		extend_update_bounds(pos);
	}
}

void Matrix::fill(uint32_t glyph, const Style &style, const Point &from, const Point &to) {
	int row0 = from.y, row1 = to.y;
	for (int row = row0; row <= row1; row++) {
		int col0 = (row == row0) ? from.x : 1;
		int col1 = (row == row1) ? to.x : m_size.x;
		for (int col = col0; col <= col1; col++) {
			set(glyph, style, Point{col, row});
		}
	}
}

void Matrix::scroll(uint32_t glyph, const Style &style, const Rect &r, int downward, int rightward) {
	// Abort if there is nothing to do
	if ((downward == 0 && rightward == 0) || !r.valid()) {
		return;
	}

	// Copy cells by calling "set" (TODO: optimize)
	const int x0 = (rightward >= 0) ? r.x0 : r.x1;
	const int x1 = (rightward >= 0) ? r.x1 : r.x0;
	const int y0 = (downward >= 0) ? r.y0 : r.y1;
	const int y1 = (downward >= 0) ? r.y1 : r.y0;
	const int dir_x = (x1 >= x0) ? 1 : -1;
	const int dir_y = (y1 >= y0) ? 1 : -1;
	for (int y_tar = y0; dir_y * y_tar <= dir_y * y1; y_tar += dir_y) {
		const int y_src = y_tar + downward;
		if (y_src < r.y0 || y_src > r.y1) {
			Cell c;
			c.style = style;
			std::fill(m_cells[y_tar - 1].begin() + r.x0 - 1, m_cells[y_tar - 1].begin() + r.x1, c);
		} else {
			for (int x_tar = x0; dir_x * x_tar <= dir_x * x1; x_tar += dir_x) {
				Cell &c_tar = m_cells[y_tar - 1][x_tar - 1];
				const int x_src = x_tar + rightward;
				if (x_src < r.x0 || x_src > r.x1) {
					Cell c;
					c.style = style;
					c_tar = c;
				} else {
					const Cell &c_src = m_cells[y_src - 1][x_src - 1];
					c_tar = c_src;
					c_tar.dirty = true;
					c_tar.cursor = false;
				}
			}
		}
	}

	// Update the y-coordinates of all stored old positions
	m_pos_old.y -= downward;
	m_pos_old.x -= rightward;

	// Set the update bounds to the entire screen rectangle
	m_update_bounds = Rect{1, 1, m_size.x, m_size.y};
}

void Matrix::set_alternative_buffer_active(bool active) {
	if (active != m_alternative_buffer_active) {
		m_alternative_buffer_active = active;
		std::swap(m_cells, m_cells_alt);
		for (size_t y = 0; y < m_cells.size(); y++) {
			for (size_t x = 0; x < m_cells[y].size(); x++) {
				m_cells[y][x].dirty = true;
			}
		}
	}
}

void Matrix::commit(std::vector<CellUpdate> &updates) {
	// Remove the "cursor" flag from the cell that last had the cursor
	if (m_cursor_visible_old && valid(m_pos_old)) {
		Cell &c = m_cells[m_pos_old.y - 1][m_pos_old.x - 1];
		c.cursor = false;
		c.dirty = true;
		extend_update_bounds(m_pos_old);
	}

	// Add the "cursor" flag to the cell that currently has the cursor
	if (m_cursor_visible && valid(m_pos)) {
		Cell &c = m_cells[m_pos.y - 1][m_pos.x - 1];
		c.cursor = true;
		c.dirty = true;
		extend_update_bounds(m_pos);
	}

	// Scan the cells within the update rectangle for updates
	for (int y = m_update_bounds.y0; y <= m_update_bounds.y1; y++) {
		for (int x = m_update_bounds.x0; x <= m_update_bounds.x1; x++) {
			// Fetch references at the current and the old cell content
			Cell &cell = m_cells[y - 1][x - 1];
			Cell &cell_old = m_cells_old[y - 1][x - 1];

			// Check whether we actually need to update the cell
			if (cell.needs_update(cell_old)) {
				updates.emplace_back(CellUpdate{Point{x, y}, cell, cell_old});
			}

			// Reset the "dirty" flag and copy the current cell content to the
			// old cell content
			cell.dirty = false;
			cell_old = cell;
		}
	}

	// Backup the old cursor position and the
	m_pos_old = m_pos;
	m_cursor_visible_old = m_cursor_visible;
}
}  // namespace inktty
