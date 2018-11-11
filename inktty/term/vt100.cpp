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

#include <vtparse/vtparse.h>

#include <inktty/term/vt100.hpp>
#include <inktty/utils/utf8.hpp>

namespace inktty {
/******************************************************************************
 * Helper functions                                                           *
 ******************************************************************************/

template <typename T>
static T clip(T v, T min, T max) {
	return (v < min) ? min : ((v > max) ? max : v);
}

/******************************************************************************
 * Class VT100::Impl                                                          *
 ******************************************************************************/

class VT100::Impl {
private:
	Matrix &m_matrix;
	UTF8Decoder m_utf8;
	vtparse_t m_vtparse;
	Style m_style;

	void update_cursor(int &row, int &col) {
		// Update the cursor location, scroll if necessary
		if (col >= int(m_matrix.cols())) {
			col = 0;
			row++;
		}
		if (row >= int(m_matrix.rows())) {
			m_matrix.scroll();
			(row)--;
		}
		m_matrix.set_cursor_position(row, col);
	}

	bool handle_execute(char ch, int &row, int &col) {
		switch (ch) {
			case 0x00:
			case 0x01:
			case 0x02:
			case 0x03:
			case 0x04:
			case 0x05:
			case 0x06:
				return false;
			case 0x07:
				// Bell. Currently unhandled.
				return true;
			case 0x08:
				// Backspace. Go to the previous column.
				if (col > 0) {
					(col)--;
				}
				return true;
			case 0x09:
				// Horizontal tab. TODO: Implement proper tabs
				(col) = (((col) + 8) / 8) * 8;
				return true;
			case 0x0A:
				(row)++;
				return true;
			case 0x0B:
			case 0x0C:
				// Vertical tab, form feed. Go to next line and column.
				// This is what gnome-terminal seems to do
				(row)++, (col)++;
				return true;
			case 0x0D:
				(col) = 0;
				return true;
			case 0x0E:
			case 0x0F:
			case 0x10:
			case 0x11:
			case 0x12:
			case 0x13:
			case 0x14:
			case 0x15:
			case 0x16:
			case 0x17:
			case 0x18:
			case 0x19:
			case 0x1A:
				return false;
			case 0x1B:
				// TODO: Ansi Escape Sequence begins here
				return true;
			case 0x1C:
			case 0x1D:
			case 0x1E:
			case 0x1F:
				return false;
			case 0x7F:
				return true;
			default:
				return false;
		}
	}

	void handle_codepoint(uint32_t codepoint, bool replaces_last, int &row,
	                      int &col) {
		// "replaces_last" indicates that the current codepoint
		// replaces the last one
		if (replaces_last && col > 0) {
			col--;
		}
		m_matrix.set_cell(row, col, codepoint, m_style);
		col++;
		update_cursor(row, col);
	}

	void handle_print(const uint8_t *begin, const uint8_t *end, int &row,
	                  int &col) {
		UTF8Decoder::Status res;
		for (uint8_t const *data = begin; data < end; data++) {
			if ((res = m_utf8.feed(*data))) {
				if (res.error) {
					// TODO
					continue;
				}
				handle_codepoint(res.codepoint, res.replaces_last, row, col);
			}
		}
	}

	bool read_sgr_color(Color &c, const int *params, int num_params, int *i, int offs, int color_offs) {
		const int *pp = &params[*i];
		const int p0 = pp[0] - offs;
		const int n_params_rem = num_params - *i;
		if (p0 <= 7) {
			c = Color(p0 + color_offs);
			return true;
		} else if (p0 == 8) {
			if (n_params_rem >= 3 && pp[1] == 5) {
				*i += 2; /* Note: i is always increased by one in handle_sgr */
				c = Color(pp[2]);
				return true;
			}
			if (n_params_rem >= 5 && pp[1] == 2) {
				*i += 4; /* Note: i is always increased by one in handle_sgr */
				c = Color(RGBA(pp[2], pp[3], pp[4]));
				return true;
			}
		}
		return false;
	}

	void handle_sgr(const int *params, int num_params) {
		/* If no parameters are given, reset the style */
		if (num_params == 0) {
			m_style = Style();
		}
		int i = 0;
		while (i < num_params) {
			const int p = params[i];
			if (p == 0) {
				m_style = Style();
			} else if (p == 1) {
				m_style.bold = true;
			} else if (p == 2) {
				/* TODO: Faint */
			} else if (p == 3) {
				m_style.italic = true;
			} else if (p == 4) {
				m_style.underline = true;
			} else if (p >= 5 && p <= 6) {
				/* Blink is not implemented */
			} else if (p == 7) {
				m_style.inverse = true;
			} else if (p == 8) {
				/* TODO: Conceal */
			} else if (p == 9) {
				m_style.strikethrough = true;
			} else if (p >= 10 && p <= 20) {
				/* Font selection: not implemented */
			} else if (p == 21) {
				/* TODO: Double underline */
			} else if (p == 22) {
				m_style.bold = false;
				/* TODO: Faint = false */
			} else if (p == 23) {
				m_style.italic = false;
			} else if (p == 24) {
				m_style.underline = false;
			} else if (p >= 25 && p <= 26) {
				/* Blink is not implemented */
			} else if (p == 27) {
				m_style.inverse = false;
			} else if (p == 28) {
				/* TODO: Conceal off */
			} else if (p == 29) {
				m_style.strikethrough = false;
			} else if (p >= 30 && p <= 38) {
				if (!read_sgr_color(m_style.fg, params, num_params, &i, 30, m_style.bold ? 8 : 0)) {
					return;
				}
			} else if (p == 39) {
				m_style.fg = Color(7);
			} else if (p >= 40 && p <= 48) {
				m_style.transparent = false;
				if (!read_sgr_color(m_style.bg, params, num_params, &i, 40, 0)) {
					return;
				}
			} else if (p == 49) {
				m_style.transparent = true;
				m_style.bg = Color(0);
			} else if (p >= 90 && p <= 97) {
				if (!read_sgr_color(m_style.fg, params, num_params, &i, 90, 8)) {
					return;
				}
			} else if (p >= 100 && p <= 107) {
				m_style.transparent = false;
				if (!read_sgr_color(m_style.bg, params, num_params, &i, 100, 8)) {
					return;
				}
			}
			i++;
		}
	}

	void handle_csi_dispatch(char ch, const int *params, int num_params,
	                         const uint8_t *intermediate_chars,
	                         int num_intermediate_chars, int &row, int &col) {
		bool needs_cursor_update = false;
		switch (ch) {
			case 'A':
			case 'B':
			case 'C':
			case 'D': {
				const int n = num_params ? params[0] : 1;
				const int dir_row = (ch == 'A') ? -n : ((ch == 'B' ? n : 0));
				const int dir_col = (ch == 'C') ? -n : ((ch == 'D' ? n : 0));
				row = clip<int>(row + dir_row, 0, m_matrix.rows() - 1);
				col = clip<int>(col + dir_col, 0, m_matrix.cols() - 1);
				needs_cursor_update = true;
				break;
			}
			case 'E':
			case 'F': {
				const int n = num_params ? params[0] - 1 : 1;
				const int dir_row = (ch == 'E') ? n : -n;
				row = clip<int>(row + dir_row, 0, m_matrix.rows() - 1);
				col = 0;
				needs_cursor_update = true;
				break;
			}
			case 'G': {
				col = num_params ? params[0] - 1 : 0;
				needs_cursor_update = true;
				break;
			}
			case 'H':
			case 'f': {
				const int n = (num_params > 0) ? (params[0] - 1) : 0;
				const int m = (num_params > 1) ? (params[1] - 1) : 0;
				row = clip<int>(n, 0, m_matrix.rows() - 1);
				col = clip<int>(m, 0, m_matrix.cols() - 1);
				needs_cursor_update = true;
				break;
			}
			case 'J':
			case 'K':
				/* Erase: TODO */
				break;
			case 'S':
			case 'T':
				/* Scroll: TODO */
				break;
			case 'm':
				handle_sgr(params, num_params);
				break;
			case 'i':
				/* AUX ON/OFF. Not implemented */
				break;
			case 'n':
				/* Device status report. TODO */
				break;
			case 's':
				/* Save cursor location */
				break;
			case 'r':
				/* Reset cursor location */
				break;
		}
		if (needs_cursor_update) {
			update_cursor(row, col);
		}
	}

public:
	Impl(Matrix &matrix) : m_matrix(matrix) { reset(); }

	void reset() {
		m_matrix.reset();
		m_utf8.reset();
		vtparse_init(&m_vtparse);
		m_style = Style();
	}

	void write(uint8_t *buf, unsigned int buf_len) {
		// Fetch the current cursor position
		int row = m_matrix.cursor_row(), col = m_matrix.cursor_col();

		while (true) {
			unsigned int n_read = vtparse_parse(&m_vtparse, buf, buf_len);
			if (!vtparse_has_event(&m_vtparse)) {
				break;
			}
			switch (m_vtparse.action) {
				case VTPARSE_ACTION_EXECUTE:
					if (handle_execute(m_vtparse.ch, row, col)) {
						update_cursor(row, col);
					}
					break;
				case VTPARSE_ACTION_PRINT:
					handle_print(m_vtparse.data_begin, m_vtparse.data_end, row,
					             col);
					break;
				case VTPARSE_ACTION_CSI_DISPATCH:
					handle_csi_dispatch(
					    m_vtparse.ch, m_vtparse.params, m_vtparse.num_params,
					    m_vtparse.intermediate_chars,
					    m_vtparse.num_intermediate_chars, row, col);
					break;
				case VTPARSE_ACTION_ESC_DISPATCH:
					break;
				case VTPARSE_ACTION_PUT:
					break;
				case VTPARSE_ACTION_OSC_START:
					break;
				case VTPARSE_ACTION_OSC_PUT:
					break;
				case VTPARSE_ACTION_OSC_END:
					break;
				case VTPARSE_ACTION_HOOK:
					break;
				case VTPARSE_ACTION_UNHOOK:
					break;
				default:
					break;
			}
			buf += n_read, buf_len -= n_read;
		}
	}
};

/******************************************************************************
 * Class VT100                                                                *
 ******************************************************************************/

VT100::VT100(Matrix &matrix) : m_impl(new Impl(matrix)) {}

VT100::~VT100() {}

void VT100::reset() { m_impl->reset(); }

void VT100::write(uint8_t *buf, unsigned int buf_len) {
	m_impl->write(buf, buf_len);
}

}  // namespace inktty
