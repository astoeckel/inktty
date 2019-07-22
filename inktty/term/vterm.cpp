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

#include <utf8proc/utf8proc.h>
#include <vterm.h>
#include <inktty/term/vterm.hpp>

namespace inktty {
/******************************************************************************
 * Class VTerm::Impl                                                          *
 ******************************************************************************/

class VTerm::Impl {
private:
	Matrix &m_matrix;
	Style m_style;
	::VTerm *m_vt;
	::VTermState *m_vt_state;

	/**
	 * Writes a single glyph to the character matrix.
	 */
	static int vterm_putglyph(VTermGlyphInfo *info, VTermPos pos, void *user) {
		Impl &self = *static_cast<Impl *>(user);

		// Count the number of characters in the buffer
		ssize_t len;
		for (len = 0; info->chars[len]; len++)
			;

		// Try to combine multiple glyphs into a single one
		uint32_t glyph = 0;
		if (utf8proc_normalize_utf32(
		        (utf8proc_int32_t *)info->chars, len,
		        static_cast<utf8proc_option_t>(UTF8PROC_STRIPCC |
		                                       UTF8PROC_COMPOSE)) > 0) {
			glyph = info->chars[0];
		}

		// Set the glyph in the matrix
		self.m_matrix.set(glyph, self.m_style, Point{pos.col + 1, pos.row + 1});
		return 1;
	}

	/**
	 * Moves the cursor to the indicated location.
	 */
	static int vterm_movecursor(VTermPos pos, VTermPos oldpos, int visible,
	                            void *user) {
		Impl &self = *static_cast<Impl *>(user);
		self.m_matrix.move_abs(pos.row + 1, pos.col + 1);
		self.m_matrix.cursor_visible(visible);
		return 1;
	}

	/**
	 * Scrolls the specified rectangle in the given direction.
	 */
	static int vterm_scrollrect(VTermRect rect, int downward, int rightward,
	                            void *user) {
		Impl &self = *static_cast<Impl *>(user);
		self.m_matrix.scroll(0, self.m_style,
		                     {rect.start_col + 1, rect.start_row + 1,
		                      rect.end_col, rect.end_row},
		                     downward, rightward);
		return 1;
	}

	static int vterm_moverect(VTermRect dest, VTermRect src, void *user) {
		Impl &self = *static_cast<Impl *>(user);
		(void)self;  // TODO
		return 1;
	}

	static int vterm_erase(VTermRect rect, int selective, void *user) {
		Impl &self = *static_cast<Impl *>(user);
		self.m_matrix.fill(0, self.m_style,
		                   {rect.start_col + 1, rect.start_row + 1},
		                   {rect.end_col, rect.end_row});
		return 1;
	}

	static int vterm_initpen(void *user) {
		Impl &self = *static_cast<Impl *>(user);
		self.m_style = Style{};
		return 1;
	}

	static Color vterm_convert_color(const VTermColor &c) {
		if (VTERM_COLOR_IS_INDEXED(&c)) {
			return Color(c.indexed.idx);
		} else if (VTERM_COLOR_IS_RGB(&c)) {
			return Color(RGBA{c.rgb.red, c.rgb.green, c.rgb.blue, 255U});
		}
		return Color(0);
	}

	static int vterm_setpenattr(VTermAttr attr, VTermValue *val, void *user) {
		Impl &self = *static_cast<Impl *>(user);
		switch (attr) {
			case VTERM_ATTR_BOLD:
				self.m_style.bold = val->boolean;
				break;
			case VTERM_ATTR_UNDERLINE:
				self.m_style.underline = val->number;
				break;
			case VTERM_ATTR_ITALIC:
				self.m_style.italic = val->boolean;
				break;
			case VTERM_ATTR_BLINK:
				/* Not supported. */
				break;
			case VTERM_ATTR_REVERSE:
				self.m_style.inverse = val->boolean;
				break;
			case VTERM_ATTR_STRIKE:
				self.m_style.strikethrough = val->boolean;
				break;
			case VTERM_ATTR_FONT:
				/* Not supported. */
				break;
			case VTERM_ATTR_FOREGROUND:
				self.m_style.fg = vterm_convert_color(val->color);
				self.m_style.default_fg = VTERM_COLOR_IS_DEFAULT_FG(&val->color);
				break;
			case VTERM_ATTR_BACKGROUND:
				self.m_style.bg = vterm_convert_color(val->color);
				self.m_style.default_bg = VTERM_COLOR_IS_DEFAULT_BG(&val->color);
				break;
			default:
				break; /* Ignore everything else */
		}
		return 1;
	}

	static int vterm_settermprop(VTermProp prop, VTermValue *val, void *user) {
		Impl &self = *static_cast<Impl *>(user);
		switch (prop) {
			case VTERM_PROP_CURSORVISIBLE:
				break;
			case VTERM_PROP_CURSORBLINK:
				/* Not supported */
				break;
			case VTERM_PROP_ALTSCREEN:
				self.m_matrix.set_alternative_buffer_active(val->boolean);
				break;
			case VTERM_PROP_TITLE:
				/* Not supported */
				break;
			case VTERM_PROP_ICONNAME:
				/* Not supported */
				break;
			case VTERM_PROP_REVERSE:
				/* Not supported */
				break;
			case VTERM_PROP_CURSORSHAPE:
				/* TODO */
				break;
			case VTERM_PROP_MOUSE:
				/* TODO */
				break;
			default:
				break; /* Ignore everything else */
		}
		return 1;
	}

	static int vterm_bell(void *user) {
		Impl &self = *static_cast<Impl *>(user);
		(void)self;  // TODO
		return 1;
	}

	static int vterm_resize(int rows, int cols, VTermPos *delta, void *user) {
		Impl &self = *static_cast<Impl *>(user);
		(void)self;  // TODO
		return 1;
	}

	static int vterm_setlineinfo(int row, const VTermLineInfo *newinfo,
	                             const VTermLineInfo *oldinfo, void *user) {
		Impl &self = *static_cast<Impl *>(user);
		(void)self;  // TODO
		return 1;
	}

	static VTermModifier vterm_keymod(bool shift, bool ctrl, bool alt) {
		return static_cast<VTermModifier>(
		    (shift ? VTERM_MOD_SHIFT : VTERM_MOD_NONE) |
		    (ctrl ? VTERM_MOD_CTRL : VTERM_MOD_NONE) |
		    (alt ? VTERM_MOD_ALT : VTERM_MOD_NONE));
	}

	static VTermKey vterm_key(Event::Key key) {
		switch (key) {
			case Event::Key::NONE:
				return VTERM_KEY_NONE;
			case Event::Key::ENTER:
				return VTERM_KEY_ENTER;
			case Event::Key::TAB:
				return VTERM_KEY_TAB;
			case Event::Key::BACKSPACE:
				return VTERM_KEY_BACKSPACE;
			case Event::Key::ESCAPE:
				return VTERM_KEY_ESCAPE;
			case Event::Key::UP:
				return VTERM_KEY_UP;
			case Event::Key::DOWN:
				return VTERM_KEY_DOWN;
			case Event::Key::LEFT:
				return VTERM_KEY_LEFT;
			case Event::Key::RIGHT:
				return VTERM_KEY_RIGHT;
			case Event::Key::INS:
				return VTERM_KEY_INS;
			case Event::Key::DEL:
				return VTERM_KEY_DEL;
			case Event::Key::HOME:
				return VTERM_KEY_HOME;
			case Event::Key::END:
				return VTERM_KEY_END;
			case Event::Key::PAGE_UP:
				return VTERM_KEY_PAGEUP;
			case Event::Key::PAGE_DOWN:
				return VTERM_KEY_PAGEDOWN;
			case Event::Key::F1:
				return static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 1);
			case Event::Key::F2:
				return static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 2);
			case Event::Key::F3:
				return static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 3);
			case Event::Key::F4:
				return static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 4);
			case Event::Key::F5:
				return static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 5);
			case Event::Key::F6:
				return static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 6);
			case Event::Key::F7:
				return static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 7);
			case Event::Key::F8:
				return static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 8);
			case Event::Key::F9:
				return static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 9);
			case Event::Key::F10:
				return static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 10);
			case Event::Key::F11:
				return static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 11);
			case Event::Key::F12:
				return static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 12);
			case Event::Key::KP_0:
				return VTERM_KEY_KP_0;
			case Event::Key::KP_1:
				return VTERM_KEY_KP_1;
			case Event::Key::KP_2:
				return VTERM_KEY_KP_2;
			case Event::Key::KP_3:
				return VTERM_KEY_KP_3;
			case Event::Key::KP_4:
				return VTERM_KEY_KP_4;
			case Event::Key::KP_5:
				return VTERM_KEY_KP_5;
			case Event::Key::KP_6:
				return VTERM_KEY_KP_6;
			case Event::Key::KP_7:
				return VTERM_KEY_KP_7;
			case Event::Key::KP_8:
				return VTERM_KEY_KP_8;
			case Event::Key::KP_9:
				return VTERM_KEY_KP_9;
			case Event::Key::KP_MULT:
				return VTERM_KEY_KP_MULT;
			case Event::Key::KP_PLUS:
				return VTERM_KEY_KP_PLUS;
			case Event::Key::KP_COMMA:
				return VTERM_KEY_KP_COMMA;
			case Event::Key::KP_MINUS:
				return VTERM_KEY_KP_MINUS;
			case Event::Key::KP_PERIOD:
				return VTERM_KEY_KP_PERIOD;
			case Event::Key::KP_DIVIDE:
				return VTERM_KEY_KP_DIVIDE;
			case Event::Key::KP_ENTER:
				return VTERM_KEY_KP_ENTER;
			case Event::Key::KP_EQUAL:
				return VTERM_KEY_KP_EQUAL;
		}
		return VTERM_KEY_NONE;
	}

	static const VTermStateCallbacks callbacks;

public:
	Impl(Matrix &matrix) : m_matrix(matrix) {
		m_vt = vterm_new(matrix.size().y, matrix.size().x);
		vterm_set_utf8(m_vt, true);

		m_vt_state = vterm_obtain_state(m_vt);
		vterm_state_set_callbacks(m_vt_state, &callbacks, this);
		vterm_state_set_bold_highbright(m_vt_state, true);
		vterm_state_reset(m_vt_state, 1);
	}

	~Impl() { vterm_free(m_vt); }

	void reset() {
		/* Use index 7/0 as default fg/bg colors */
		VTermColor vt_default_fg;
		VTermColor vt_default_bg;
		vterm_color_indexed(&vt_default_fg, 7);
		vterm_color_indexed(&vt_default_bg, 0);
		vterm_state_set_default_colors(m_vt_state, &vt_default_fg,
		                               &vt_default_bg);

		vterm_state_reset(m_vt_state, 1);
		m_matrix.reset();
		m_style = Style{};
	}

	void send_key(Event::Key key, bool shift, bool ctrl, bool alt) {
		vterm_keyboard_key(m_vt, vterm_key(key),
		                   vterm_keymod(shift, ctrl, alt));
	}

	void send_char(uint32_t unichar, bool shift, bool ctrl, bool alt) {
		vterm_keyboard_unichar(m_vt, unichar, vterm_keymod(shift, ctrl, alt));
	}

	void receive_from_pty(const uint8_t *buf, size_t buf_len) {
		vterm_input_write(m_vt, (const char *)buf, buf_len);
	}

	size_t send_to_pty(uint8_t *buf, size_t buf_len) {
		return vterm_output_read(m_vt, (char *)buf, buf_len);
	}
};

const VTermStateCallbacks VTerm::Impl::callbacks{
    VTerm::Impl::vterm_putglyph,   VTerm::Impl::vterm_movecursor,
    VTerm::Impl::vterm_scrollrect, VTerm::Impl::vterm_moverect,
    VTerm::Impl::vterm_erase,      VTerm::Impl::vterm_initpen,
    VTerm::Impl::vterm_setpenattr, VTerm::Impl::vterm_settermprop,
    VTerm::Impl::vterm_bell,       VTerm::Impl::vterm_resize,
    VTerm::Impl::vterm_setlineinfo};

/******************************************************************************
 * Class VT100                                                                *
 ******************************************************************************/

VTerm::VTerm(Matrix &matrix) : m_impl(new Impl(matrix)) {}

VTerm::~VTerm() {}

void VTerm::reset() { m_impl->reset(); }

void VTerm::send_key(Event::Key key, bool shift, bool ctrl, bool alt) {
	m_impl->send_key(key, shift, ctrl, alt);
}

void VTerm::send_char(uint32_t unichar, bool shift, bool ctrl, bool alt) {
	m_impl->send_char(unichar, shift, ctrl, alt);
}

void VTerm::receive_from_pty(const uint8_t *buf, size_t buf_len) {
	m_impl->receive_from_pty(buf, buf_len);
}

size_t VTerm::send_to_pty(uint8_t *buf, size_t buf_len) {
	return m_impl->send_to_pty(buf, buf_len);
}

}  // namespace inktty
