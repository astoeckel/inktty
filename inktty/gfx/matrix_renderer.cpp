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

#include <inktty/gfx/epaper_emulation.hpp>
#include <inktty/gfx/matrix_renderer.hpp>
#include <inktty/utils/logger.hpp>

namespace inktty {

class MatrixRenderer::Impl {
private:
	/**
	 * The Cell structure contains information about the status of a
	 * cell as it is currently displayed on screen.
	 */
	struct Cell {
		/**
		 * Last known state of the cell as stored in the underlying Matrix
		 * structure.
		 */
		Matrix::Cell cell;

		/**
		 * The time since the cell was last updated. This is reset to zero
		 * whenever the cell is being redrawn.
		 */
		uint32_t last_update;

		/**
		 * The global number of update operations since the cell has last been
		 * redrawn. Since the content shown on epaper displays degrades over
		 * time when other parts of the screen are updated, this counter is
		 * used to regularly update the screen.
		 */
		uint32_t operation_counter;

		/**
		 * A flag indicating whether or not the cell has last been drawn in
		 * the low quality mode.
		 */
		bool is_low_quality : 1;

		/**
		 * A flag indicating whether the cell has been drawn in the high quality
		 * mode.
		 */
		bool is_high_quality : 1;

		/**
		 * A flag indicating whether the cell needs a refresh because of one of
		 * the global update rules.
		 */
		bool is_overdue : 1;

		/**
		 * A flag indicating whether or not the cell itself needs to be redrawn.
		 */
		bool is_dirty : 1;

		/**
		 * Initialises the cell metadata to default values.
		 */
		Cell()
		    : last_update(0),
		      operation_counter(0),
		      is_low_quality(false),
		      is_high_quality(true),
		      is_overdue(true),
		      is_dirty(false) {}
	};

	std::vector<std::vector<Cell>> m_cells;

	Rect m_update_bounds;

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

	RectangleMerger m_merger;

	/**
	 * Used internally to recompute the number of cells and to initialize the
	 * cell metadata in case the underlying display geometry changes.
	 */
	void update_geometry() {
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

		/* Allocate the Cell structure */
		m_cells.clear();
		for (size_t i = 0; i < m_rows; i++) {
			m_cells.emplace_back(std::vector<Cell>(m_cols, Cell()));
		}

		/* Resize the underlying matrix instance */
		m_matrix.resize(m_rows, m_cols);

		/* The geometry has been updated, prevent unecessary calls to this
		   function. */
		m_needs_geometry_update = false;
	}

	Rect get_coords(size_t row, size_t col) {
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

	Rect draw_cell(size_t row, size_t col, const Matrix::Cell &cell, bool erase,
	               bool low_quality = true) {
		/* Fetch foreground and background colour */
		const auto &cc = m_config.colors;  // color config
		Color cfg = cell.style.fg, cbg = cell.style.bg;
		if (cc.use_bright_on_bold && cell.style.bold &&
		    cell.style.fg.is_indexed() && cell.style.fg.idx() < 8) {
			cfg = Color(cell.style.fg.idx() + 8);
		}

		/* Convert the colours to RGBA */
		RGBA fg, bg;
		if (cell.style.default_fg) {
			fg = cc.default_fg;
		} else {
			fg = cfg.rgb(cc.palette);
		}
		if (cell.style.default_bg) {
			bg = cc.default_bg;
		} else {
			bg = cbg.rgb(cc.palette);
		}
		if (cell.cursor ^ cell.style.inverse) {
			std::swap(fg, bg);
		}

		Rect r = get_coords(row, col);
		Rect gr = r;
		const GlyphBitmap *g = nullptr;
		if (low_quality) {
			const uint8_t g_fg = epaper_emulation::rgba_to_greyscale(fg);
			const uint8_t g_bg = epaper_emulation::rgba_to_greyscale(bg);
			if (!erase) {
				m_display.fill_dither(Display::Layer::Background, g_bg, r);
			}
			if (fg != bg) {
				g = m_font.render(cell.glyph, m_font_size, true, m_orientation);
			}
			if (g_fg >= g_bg) {
				fg = RGBA::White;
			} else {
				fg = RGBA::Black;
			}
		} else {
			if (!erase) {
				m_display.fill(Display::Layer::Background, bg, r);
			}
			g = m_font.render(cell.glyph, m_font_size, false, m_orientation);
		}

		if (g) {
			gr = Rect::sized(r.x0 + g->x, r.y0 + g->y, g->w, g->h);
			if (low_quality && bg != RGBA::White && bg != RGBA::Black) {
				Rect gr2 =
				    Rect::sized(r.x0 + g->x + 1, r.y0 + g->y + 1, g->w, g->h);
				m_display.blit(Display::Layer::Presentation, ~fg, g->buf(),
				               g->stride, gr2,
				               erase ? Display::DrawMode::Erase
				                     : Display::DrawMode::Write);
				r = r.grow(gr2);
			}
			m_display.blit(
			    Display::Layer::Presentation, fg, g->buf(), g->stride, gr,
			    erase ? Display::DrawMode::Erase : Display::DrawMode::Write);
		}
		return r.grow(gr);
	}

public:
	Impl(const Configuration &config, Font &font, Display &display,
	     Matrix &matrix, unsigned int font_size, unsigned int orientation)
	    : m_config(config),
	      m_font(font),
	      m_display(display),
	      m_matrix(matrix),
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
	}

	void draw(bool redraw, int dt) {
		/* Check whether the geometry needs to be updated */
		if (m_needs_geometry_update) {
			update_geometry(); /* Resets m_needs_geometry_update */
		}

		/* If the redraw flag is set, mark all cells as dirty by resetting the
		   cell metadata and thus marking the cell as "overdue". */
		if (redraw) {
			for (size_t y = 0; y < m_rows; y++) {
				for (size_t x = 0; x < m_cols; x++) {
					m_cells[y][x] = Cell();
					m_update_bounds = m_update_bounds.grow(Point(x, y));
				}
			}
		}

		/* Increment the last_update timer */
		for (size_t y = 0; y < m_rows; y++) {
			for (size_t x = 0; x < m_cols; x++) {
				m_cells[y][x].last_update += dt;
			}
		}

		/* Collect all updates from the underlying cell matrix. */
		std::vector<Point> updates;
		m_matrix.commit(updates);
		for (const Point &p: updates) {
			if (p.y <= int(m_rows) && p.x <= int(m_cols)) {
				m_cells[p.y - 1][p.x - 1].is_dirty = true;
				m_update_bounds = m_update_bounds.grow(Point(p.x - 1, p.y - 1));
			}
		}

		/* Update cells depending on a global update rule, such as a redraw
		   timeout. */
		constexpr uint32_t redraw_timeout_low = 250;
		constexpr uint32_t redraw_timeout_high = 1000;
		constexpr uint32_t update_counter_threshold_low = 1000;
		constexpr uint32_t update_counter_threshold_high = 2000;
		uint32_t update_counter_threshold = update_counter_threshold_high;
		uint32_t redraw_timeout = redraw_timeout_high;
		for (size_t y = 0; y < m_rows; y++) {
			for (size_t x = 0; x < m_cols; x++) {
				if (m_cells[y][x].operation_counter >
				    update_counter_threshold) {
					update_counter_threshold = update_counter_threshold_low;
				}
				if (m_cells[y][x].last_update >
				    redraw_timeout) {
					redraw_timeout = redraw_timeout_low;
					break;
				}
			}
		}
		for (size_t y = 0; y < m_rows; y++) {
			for (size_t x = 0; x < m_cols; x++) {
				Cell &c = m_cells[y][x];
				const bool rule_counter =
				    c.operation_counter >= update_counter_threshold;
				const bool rule_timeout =
				    c.is_low_quality && c.last_update >= redraw_timeout;
				if (rule_counter || rule_timeout) {
					m_cells[y][x].is_overdue = true;
					m_update_bounds = m_update_bounds.grow(Point(x, y));
				}
			}
		}

		/* Cancel if there are no updates scheduled */
		if (!m_update_bounds.valid()) {
			return;
		}

		/* There is going to be at least one draw operation; update the global
		   operation counter. */
		for (size_t y = 0; y < m_rows; y++) {
			for (size_t x = 0; x < m_cols; x++) {
				m_cells[y][x].operation_counter++;
			}
		}

		m_display.lock(); /* TODO update screen size */

		/* Pass 1: Redraw all dirty cells in low quality mode */
		const auto &matrix_cells = m_matrix.cells();
		m_merger.reset();
		for (int y = m_update_bounds.y0; y <= m_update_bounds.y1; y++) {
			for (int x = m_update_bounds.x0; x <= m_update_bounds.x1; x++) {
				/* Fetch the cell, skip it if it was not dirty */
				Cell &c = m_cells[y][x];
				if (!c.is_dirty) {
					continue;
				}

				/* Fetch the old and new cell content */
				const Matrix::Cell &c_old = m_cells[y][x].cell;
				const Matrix::Cell &c_new = matrix_cells[y][x];

				/* Erase the cell */
				const Rect r1 =
				    draw_cell(y, x, c_old, true, c.is_low_quality);

				/* Draw the cell in low quality mode */
				const Rect r2 =
				    draw_cell(y, x, c_new, false, true);

				/* Insert the region we touched into the rectangle merger */
				m_merger.insert(r1.grow(r2));

				/* Update the cell metadata */
				c.cell = c_new;
				c.operation_counter = 0;
				c.last_update = 0;
				c.is_high_quality = false;
				c.is_low_quality = true;
				c.is_overdue = false;
				c.is_dirty = false;
			}
		}

		/* Merge all rectangles and commit them; draw them in low-quality mode*/
		m_merger.merge();
		for (const Rect &r : m_merger) {
			m_display.commit(
			    r, UpdateMode(UpdateMode::Identity, UpdateMode::SourceMono));
		}

		/* Pass 2: Redraw all overdue cells in high quality mode */
		m_merger.reset();
		for (int y = m_update_bounds.y0; y <= m_update_bounds.y1; y++) {
			for (int x = m_update_bounds.x0; x <= m_update_bounds.x1; x++) {
				/* Fetch the cell, skip it if it was not dirty */
				Cell &c = m_cells[y][x];
				if (!c.is_overdue) {
					continue;
				}

				/* Fetch the old and new cell content */
				const Matrix::Cell &c_old = m_cells[y][x].cell;
				const Matrix::Cell &c_new = matrix_cells[y][x];

				/* Erase the cell */
				const Rect r1 =
				    draw_cell(y, x, c_old, true, c.is_low_quality);

				/* Draw the cell in low quality mode */
				const Rect r2 =
				    draw_cell(y, x, c_new, false, false);

				/* Insert the region we touched into the rectangle merger */
				m_merger.insert(r1.grow(r2));

				/* Update the cell metadata */
				c.cell = c_new;
				c.operation_counter = 0;
				c.last_update = 0;
				c.is_high_quality = true;
				c.is_low_quality = false;
				c.is_overdue = false;
				c.is_dirty = false;
			}
		}
		m_merger.merge();
		for (const Rect &r : m_merger) {
			m_display.commit(
			    r, UpdateMode(UpdateMode::Identity, UpdateMode::Partial));
		}

		m_display.unlock();

		/* Reset the update boundaries */
		m_update_bounds = Rect();
	}

	void set_font_size(unsigned int font_size) {
		if (font_size != m_font_size) {
			m_needs_geometry_update = true;
			m_font_size = font_size;
		}
	}

	unsigned int font_size() const { return m_font_size; }

	void set_orientation(unsigned int orientation) {
		/* There are four possible orientations */
		orientation = orientation % 4;

		/* Trigger a full update if the orientation changed */
		if (orientation != m_orientation) {
			m_display.fill(Display::Layer::Background, RGBA::Black, m_bounds);
			m_display.fill(Display::Layer::Presentation, RGBA(0, 0, 0, 0),
			               m_bounds);
			m_orientation = orientation;
			m_needs_geometry_update = true;
		}
	}

	unsigned int orientation() const { return m_orientation; }
};

/******************************************************************************
 * Class MatrixRenderer                                                       *
 ******************************************************************************/

MatrixRenderer::MatrixRenderer(const Configuration &config, Font &font,
                               Display &display, Matrix &matrix,
                               unsigned int font_size, unsigned int orientation)
    : m_impl(new Impl(config, font, display, matrix, font_size, orientation)) {}

MatrixRenderer::~MatrixRenderer() {
	// Make sure the destructor of m_impl is called.
}

void MatrixRenderer::draw(bool redraw, int dt) { m_impl->draw(redraw, dt); }

void MatrixRenderer::set_font_size(unsigned int font_size) {
	m_impl->set_font_size(font_size);
}

unsigned int MatrixRenderer::font_size() const { return m_impl->font_size(); }

void MatrixRenderer::set_orientation(unsigned int orientation) {
	m_impl->set_orientation(orientation);
}

unsigned int MatrixRenderer::orientation() const {
	return m_impl->orientation();
}

}  // namespace inktty
