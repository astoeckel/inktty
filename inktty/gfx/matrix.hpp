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

#include <inktty/gfx/color.hpp>
#include <inktty/gfx/display.hpp>
#include <inktty/gfx/font.hpp>
#include <inktty/gfx/geometry.hpp>

#ifndef INKTTY_GFX_MATRIX_HPP
#define INKTTY_GFX_MATRIX_HPP

namespace inktty {
/**
 * The Style structure tracks the text style of the TTY and is modified by
 * appropriate ANSI escape sequences.
 */
struct Style {
	/**
	 * Foreground or text colour.
	 */
	Color fg;

	/**
	 * Background colour.
	 */
	Color bg;

	/**
	 * If true (default), the background is not painted.
	 */
	bool transparent : 1;

	/**
	 * If true, the glyph is drawn with a bold font.
	 */
	bool bold : 1;

	/**
	 * If true, the glyph is drawn with an italic font.
	 */
	bool italic : 1;

	/**
	 * If true, the glyph is underlined.
	 */
	bool underline : 1;

	/**
	 * If true, the glyph is rendered striked through.
	 */
	bool strikethrough : 1;

	/**
	 * Default constructor, resets all member variables to their default value.
	 */
	Style()
	    : fg(7),
	      bg(0),
	      transparent(true),
	      bold(false),
	      italic(false),
	      underline(false),
	      strikethrough(false) {}

	/**
	 * Compares this Style instance to the given other Style instance.
	 */
	bool operator==(const Style &o) const {
		return (fg == o.fg) && (bg == o.bg) && (transparent == o.transparent) &&
		       (bold == o.bold) && (italic == o.italic) &&
		       (underline == o.underline) && (strikethrough == o.strikethrough);
	}

	bool operator!=(const Style &o) const {
		return !((*this) == o);
	}
};

class Matrix {
private:
	/**
	 * The Cell structure describes content and style of a single cell in the
	 * terminal matrix. Furthermore, it tracks information regarding the current
	 * display state of each individual cell.
	 */
	struct Cell {
		/**
		 * Enumeration describing the status of a cell regarding
		 */
		enum class Status {
			/**
			 * The cell was modified and has not yet been drawn to the screen.
			 */
			dirty,

			/**
			 * The cell has been rendered to the screen in the low-quality mode.
			 */
			low_quality,

			/**
			 * The cell has rendered in the high-quality mode.
			 */
			high_quality
		};

		/**
		 * Current Unicode glyph that is being displayed in this cell or zero if
		 * the cell is empty.
		 */
		uint32_t glyph;

		/**
		 * Current cell style.
		 */
		Style style;

		/**
		 * Current cell status.
		 */
		Status status;

		/**
		 * Generation number indicating when this cell has been written to the
		 * screen the last time. This is required since partial screen updates
		 * of a region of an E-Paper display will slowly degrade the content of
		 * other screen regions. Correspondingly, after a certain number of
		 * updates all cells need to be refreshed.
		 */
		int32_t generation;

		Cell() : glyph(0), status(Status::dirty), generation(0) {}
	};

	Font &m_font;

	Display &m_display;

	Palette m_palette;

	std::vector<std::vector<Cell>> m_cells;

	unsigned int m_font_size;

	unsigned int m_orientation;

	size_t m_cols, m_rows;

	int m_x0, m_y0, m_x1, m_y1;

	int m_pad_x, m_pad_y;

	size_t m_cell_w, m_cell_h;

	bool m_needs_geometry_update;

	void update_geometry();

	Rect get_coords(size_t row, size_t col);

public:
	Matrix(Font &font, Display &display, unsigned int font_size = 12 * 64,
	       unsigned int orientation = 0);

	/**
	 * Clears all cell content.
	 */
	void reset();

	/**
	 * Draws the matrix to the specified display instance.
	 */
	void draw();

	void update();

	/**
	 * Sets the client area to which the Matrix will be drawn in display
	 * coordinates.
	 */
	void set_area(int x0, int y0, int x1, int y1);

	void set_font_size(unsigned int font_size);

	unsigned int font_size() const { return m_font_size; }

	void set_orientation(unsigned int rotation);

	unsigned int orientation() const { return m_orientation; }

	/**
	 * Sets the content of the cell in the given row and column.
	 *
	 * @param row is the row of the cell that should be updated.
	 * @param col is the column of the cell that should be updated.
	 * @param glyph is the glyph that should be updated.
	 * @param style is the style that should be assigned to the cell.
	 */
	void set_cell(size_t row, size_t col, uint32_t glyph,
	              const Style &style);

	/**
	 * Returns the number of cells in x-direction.
	 */
	size_t cols() const { return m_cols; }

	/**
	 * Returns the number of cells in y-direction.
	 */
	size_t rows() const { return m_rows; }
};

}  // namespace inktty

#endif /* INKTTY_GFX_MATRIX_HPP */
