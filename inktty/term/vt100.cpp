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

static uint32_t DEC_SPECIAL_GRAPHICS_TABLE[] = {
    0xA0,   /* (0x5F, 95) */
    0x25C6, /* (0x60, 96) */
    0x2592, /* (0x61, 97) */
    0x2409, /* (0x62, 98) */
    0x240C, /* (0x63, 99) */
    0x240D, /* (0x64, 100) */
    0x240A, /* (0x65, 101) */
    0xB0,   /* (0x66, 102) */
    0xB1,   /* (0x67, 103) */
    0x2424, /* (0x68, 104) */
    0x240B, /* (0x69, 105) */
    0x2518, /* (0x6A, 106) */
    0x2510, /* (0x6B, 107) */
    0x250C, /* (0x6C, 108) */
    0x2514, /* (0x6D, 109) */
    0x253C, /* (0x6E, 110) */
    0x23BA, /* (0x6F, 111) */
    0x23BB, /* (0x70, 112) */
    0x2500, /* (0x71, 113) */
    0x23BC, /* (0x72, 114) */
    0x23BD, /* (0x73, 115) */
    0x251C, /* (0x74, 116) */
    0x2524, /* (0x75, 117) */
    0x2534, /* (0x76, 118) */
    0x252C, /* (0x77, 119) */
    0x2502, /* (0x78, 120) */
    0x2264, /* (0x79, 121) */
    0x2265, /* (0x7A, 122) */
    0x3C0,  /* (0x7B, 123) */
    0x2260, /* (0x7C, 124) */
    0xA3,   /* (0x7D, 125) */
    0xB7    /* (0x7E, 126) */
};

static uint8_t DEC_SPECIAL_GRAPHICS_TABLE_BEGIN = 0x5F;
static uint8_t DEC_SPECIAL_GRAPHICS_TABLE_END = 0x7E;

/******************************************************************************
 * Class VT100::Impl                                                          *
 ******************************************************************************/

class VT100::Impl {
private:
	struct Mode {
		bool dec_special_graphics;

		Mode() : dec_special_graphics(false) {}
	};

	Matrix &m_matrix;
	UTF8Decoder m_utf8;
	vtparse_t m_vtparse;
	Style m_style;
	Mode m_mode;

	bool handle_execute(char ch) {
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
				m_matrix.move_rel(0, -1);
				return true;
			case 0x09:
				// Horizontal tab. TODO: Implement proper tabs
				m_matrix.move_rel(
				    0, ((m_matrix.col() + 8) / 8) * 8 - m_matrix.col(), true, m_style);
				return true;
			case 0x0A:
				m_matrix.move_rel(1, 0, true, m_style);
				return true;
			case 0x0B:
			case 0x0C:
				// Vertical tab, form feed. Go to next line and column.
				// This is what gnome-terminal seems to do
				m_matrix.move_rel(1, 1, true, m_style);
				return true;
			case 0x0D:
				m_matrix.move_abs(m_matrix.row(), 1);
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

	void handle_codepoint(uint32_t codepoint, bool replaces_last) {}

	void handle_print(const uint8_t *begin, const uint8_t *end) {
		UTF8Decoder::Status res;
		for (uint8_t const *data = begin; data < end; data++) {
			if ((res = m_utf8.feed(*data))) {
				if (res.error) {
					// TODO
					continue;
				}

				/* Perform character substitutions */
				if (m_mode.dec_special_graphics &&
				    res.codepoint >= DEC_SPECIAL_GRAPHICS_TABLE_BEGIN &&
				    res.codepoint <= DEC_SPECIAL_GRAPHICS_TABLE_END) {
					res.codepoint = DEC_SPECIAL_GRAPHICS_TABLE
					    [res.codepoint - DEC_SPECIAL_GRAPHICS_TABLE_BEGIN];
				}

				m_matrix.write(res.codepoint, m_style, res.replaces_last);
			}
		}
	}

	bool read_sgr_color(Color &c, const int *params, int num_params, int *i,
	                    int offs, int color_offs) {
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

	void handle_esc_dispatch(char ch, const int *params, int num_params,
	                         const uint8_t *intermediate_chars,
	                         int num_intermediate_chars) {
		/*		std::cout << "ESC dispatch \"";
		        for (int i = 0; i < num_intermediate_chars; i++) {
		            std::cout << (char)intermediate_chars[i];
		        }
		        for (int i = 0; i < num_params; i++) {
		            if (i > 0) {
		                std::cout << ";";
		            }
		            std::cout << params[i];
		        }
		        std::cout << (char)ch << "\"" << std::endl;*/

		switch (ch) {
			case 'c':
				reset();
				break;
			case '0':
				if (num_intermediate_chars == 1 &&
				    intermediate_chars[0] == '(') {
					m_mode.dec_special_graphics = true;
				}
				break;
			case 'B':
				if (num_intermediate_chars == 1 &&
				    intermediate_chars[0] == '(') {
					m_mode.dec_special_graphics = false;
				}
				break;
			default:
				// std::cout << "!!!! Unhandled ESC dispatch" << std::endl;
				break;
		}
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
				if (!read_sgr_color(m_style.fg, params, num_params, &i, 30,
				                    m_style.bold ? 8 : 0)) {
					return;
				}
				m_style.default_fg = false;
			} else if (p == 39) {
				m_style.default_fg = true;
				m_style.fg = Color(7);
			} else if (p >= 40 && p <= 48) {
				if (!read_sgr_color(m_style.bg, params, num_params, &i, 40,
				                    0)) {
					return;
				}
				m_style.default_bg = false;
			} else if (p == 49) {
				m_style.default_bg = true;
				m_style.bg = Color(0);
			} else if (p >= 90 && p <= 97) {
				if (!read_sgr_color(m_style.fg, params, num_params, &i, 90,
				                    8)) {
					return;
				}
				m_style.default_fg = true;
			} else if (p >= 100 && p <= 107) {
				if (!read_sgr_color(m_style.bg, params, num_params, &i, 100,
				                    8)) {
					return;
				}
				m_style.default_bg = true;
			}
			i++;
		}
	}

	void handle_csi_dispatch(char ch, const int *params, int num_params,
	                         const uint8_t *intermediate_chars,
	                         int num_intermediate_chars) {
		switch (ch) {
			case 'A': /* Move cursor up */
				m_matrix.move_rel(-std::max(1, params[0]), 0);
				break;
			case 'e':
			case 'B': /* Move cursor down */
				m_matrix.move_rel(std::max(1, params[0]), 0);
				break;
			case 'a':
			case 'C': /* Move cursor right */
				m_matrix.move_rel(0, std::max(1, params[0]));
				break;
			case 'D': /* Move cursor left */
				m_matrix.move_rel(0, -std::max(1, params[0]));
				break;
			case 'E':
			case 'F': { /* Move cursor n rows up/down */
				const int n = num_params ? params[0] : 1;
				const int dir_row = (ch == 'E') ? n : -n;
				m_matrix.move_rel(Point(0, dir_row));
				break;
			}
			case 'G': { /* Set colum */
				m_matrix.col(num_params ? params[0] : 1);
				break;
			}
			case 'H':
			case 'f': { /* Set absolute row/column */
				const int n = (num_params > 0) ? params[0] : 1;
				const int m = (num_params > 1) ? params[1] : 1;
				m_matrix.move_abs(n, m);
				break;
			}
			case 'J': { /* Clear screen */
				switch (params[0]) {
					case 0:
						m_matrix.fill(0, m_style, m_matrix.pos(), m_matrix.size());
						break;
					case 1:
						m_matrix.fill(0, m_style, Point{1, 1}, m_matrix.pos());
						break;
					case 2:
						m_matrix.fill(0, m_style, Point{1, 1}, m_matrix.size());
						break;
				}
				break;
			}
			case 'K': { /* Clear line */
				const int row = m_matrix.row();
				const int col0 = m_matrix.col(), col1 = m_matrix.size().x;
				switch (params[0]) {
					case 0:
						m_matrix.fill(0, m_style, {row, col0}, {row, col1});
						break;
					case 1:
						m_matrix.fill(0, m_style, {row, 1}, {row, col0});
						break;
					case 2:
						m_matrix.fill(0, m_style, {row, 1}, {row, col1});
						break;
				}
				break;
			}
			case 'P': {  // Delete characters right of the cursor
				m_matrix.shift_left(0, m_style, {ccol, crow}, P(0));
				break;
			}
			case 'S':
			case 'T': {
				const int n = params[0] ? params[0] : 1;
				const int dir = (ch == 'S') ? n : -n;
				m_matrix.scroll(dir, m_style);
				break;
			}
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
		while (true) {
			unsigned int n_read = vtparse_parse(&m_vtparse, buf, buf_len);
			if (!vtparse_has_event(&m_vtparse)) {
				break;
			}
			switch (m_vtparse.action) {
				case VTPARSE_ACTION_EXECUTE:
					handle_execute(m_vtparse.ch);
					break;
				case VTPARSE_ACTION_PRINT:
					handle_print(m_vtparse.data_begin, m_vtparse.data_end);
					break;
				case VTPARSE_ACTION_ESC_DISPATCH:
					handle_esc_dispatch(m_vtparse.ch, m_vtparse.params,
					                    m_vtparse.num_params,
					                    m_vtparse.intermediate_chars,
					                    m_vtparse.num_intermediate_chars);
					break;
				case VTPARSE_ACTION_CSI_DISPATCH:
					handle_csi_dispatch(m_vtparse.ch, m_vtparse.params,
					                    m_vtparse.num_params,
					                    m_vtparse.intermediate_chars,
					                    m_vtparse.num_intermediate_chars);
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
