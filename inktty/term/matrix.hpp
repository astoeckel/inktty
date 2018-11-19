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

/**
 * @file matrix.hpp
 *
 * The Matrix class implements the ch
 *
 * @author Andreas Stöckel
 */

#ifndef INKTTY_TERM_MATRIX_HPP
#define INKTTY_TERM_MATRIX_HPP

#include <vector>

#include <inktty/utils/color.hpp>
#include <inktty/utils/geometry.hpp>

namespace inktty {
/**
 * The Style structure tracks the text style of the TTY and is modified by
 * appropriate ANSI escape sequences.
 */
struct Style {
	/**
	 * Foreground or text colour.
	 */
	RGBA fg;

	/**
	 * Background colour.
	 */
	RGBA bg;

	/**
	 * True if the foreground is not rendered.
	 */
	bool concealed : 1;

	/**
	 * If true, the glyph is drawn with a bold font.
	 */
	bool bold : 1;

	/**
	 * If true, the glyph is drawn with an italic font.
	 */
	bool italic : 1;

	/**
	 * If true, the glyph is rendered striked through.
	 */
	bool strikethrough : 1;

	/**
	 * If true, background and foreground color are inverted.
	 */
	bool inverse : 1;

	/**
	 * If zero, the glyph is not underlined, if one the glyph is singly
	 * underlined, if two the glyph is doubly underlined.
	 */
	unsigned int underline : 2;

	/**
	 * Default constructor, resets all member variables to their default value.
	 */
	Style()
	    : fg(0xF7F7F7),
	      bg(0x000000),
	      concealed(false),
	      bold(false),
	      italic(false),
	      strikethrough(false),
	      inverse(false),
	      underline(0) {}

	/**
	 * Compares this Style instance to the given other Style instance.
	 */
	bool operator==(const Style &o) const {
		return (fg == o.fg) && (bg == o.bg) && (concealed == o.concealed) &&
		       (bold == o.bold) && (italic == o.italic) &&
		       (strikethrough == o.strikethrough) && (inverse == o.inverse) &&
		       (underline == o.underline);
	}

	bool operator!=(const Style &o) const { return !((*this) == o); }
};

class Matrix {
public:
	/**
	 * The Cell structure describes content and style of a single cell in the
	 * terminal matrix. Furthermore, it tracks information regarding the current
	 * display state of each individual cell.
	 */
	struct Cell {
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
		 * If true, this cell corresponds to the current cursor location.
		 */
		bool cursor;

		/**
		 * If true, the cell has been touched.
		 */
		bool dirty;

		Cell() : glyph(0), cursor(false), dirty(true) {}

		/**
		 * Returns true if the cell changed in any significant way compared to
		 * the old cell content.
		 */
		bool needs_update(const Cell &old) const;

		/**
		 * Returns true if the cell foreground is invisible.
		 */
		bool invisible() const;
	};

	/**
	 * Data structure holding the content of all cells in the matrix.
	 */
	using CellArray = std::vector<std::vector<Cell>>;

	/**
	 * Data structure used to inform about a partial matrix update.
	 */
	struct CellUpdate {
		/**
		 * Position this CellUpdate instance is refering to.
		 */
		Point pos;

		/**
		 * The new, current content of the cell.
		 */
		Cell current;

		/**
		 * Old cell content before the update.
		 */
		Cell old;
	};

private:
	/**
	 * Cell array holding the current cell contents.
	 */
	CellArray m_cells;

	/**
	 * Cell array holding the cell content of the alternative buffer.
	 */
	CellArray m_cells_alt;

	/**
	 * Cell array holding the cell contents after the last commit action.
	 */
	CellArray m_cells_old;

	/**
	 * Current cursor location.
	 */
	Point m_pos;

	/**
	 * Last cursor write location.
	 */
	Point m_pos_last;

	/**
	 * Old cursor location.
	 */
	Point m_pos_old;

	/**
	 * Current matrix size.
	 */
	Point m_size;

	/**
	 * Flag indicating whether or not the cursor is visible at the moment.
	 */
	bool m_cursor_visible;

	/**
	 * Old "cursor visible" flag.
	 */
	bool m_cursor_visible_old;

	bool m_alternative_buffer_active;

	/**
	 * Bounding rectangle describing the area of updated cells.
	 */
	Rect m_update_bounds;

	bool valid(const Point &p) const;

	void extend_update_bounds(const Point &p);

public:
	/**
	 * Creates a new Matrix instance with the given initial size.
	 */
	Matrix(int rows = 40, int cols = 80);

	/**
	 * Returns a reference at the internal CellArray instance holding the
	 * current state of the matrix. Note that some updates only become visible
	 * visible in the CellArray after commit() has been called.
	 */
	CellArray &cells() { return m_cells; }

	/**
	 * Resets the matrix, does not change the size.
	 */
	void reset();

	/**
	 * Returns the current size of the matrix.
	 */
	Point size() const { return m_size; }

	/**
	 * Resizes the matrix to the given side. This may be called outside of a
	 * batch update.
	 */
	void resize(int rows, int cols);

	/**
	 * Marks the cursor as visible.
	 */
	void cursor_visible(bool visible) { m_cursor_visible = visible; }

	/**
	 * Marks the cursor as invisible.
	 */
	bool cursor_visible() const { return m_cursor_visible; }

	/**
	 * Returns the current cursor location.
	 */
	Point pos() const { return m_pos; }

	int row() const { return m_pos.y; }

	int col() const { return m_pos.x; }

	/**
	 * Moves the cursor to the given absolute location. Clips the cursor
	 * location to the current matrix size. (1, 1) corresponds to the upper-left
	 * corner.
	 */
	void move_abs(Point pos);

	void move_abs(int row, int col) { move_abs(Point(col, row)); }

	void col(int col) { move_abs(row(), col); }

	void row(int row) { move_abs(row, col()); }

	/**
	 * Sets the cell at the given position to contain the given glyph with the
	 * given style. If the position is out of bounds, does nothing.
	 */
	void set(uint32_t glyph, const Style &style, Point pos);

	/**
	 * Fills the screen with the given glyph and style from the given cursor
	 * location to the given cursor location.
	 */
	void fill(uint32_t glyph, const Style &style, const Point &from,
	          const Point &to);

	/**
	 * Scrolls the entire view one line up or down. Inserts a blank row with the
	 * given style.
	 */
	void scroll(uint32_t glyph, const Style &style, const Rect &r, int downward, int rightward);

	void set_alternative_buffer_active(bool active);

	/**
	 * Commits all updates to the matrix. Writes the status of all the cells
	 * that changed to the provided vector. Note that "commit" only provides a
	 * "compressed" update, i.e. if the same cell is set twice, only the last
	 * update will be present in the "updates" vector. Updates are likely not
	 * in the sequence in which the above functions were called.
	 */
	void commit(std::vector<CellUpdate> &updates);
};

}  // namespace inktty

#endif /* INKTTY_TERM_MATRIX_HPP */
