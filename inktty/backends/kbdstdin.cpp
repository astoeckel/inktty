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

#include <cerrno>
#include <cstdio>
#include <system_error>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <termkey.h>

#include <inktty/backends/kbdstdin.hpp>

namespace inktty {

/******************************************************************************
 * Class KbdStdin::Data                                                       *
 ******************************************************************************/

struct KbdStdin::Data {
	int fd_stdout;
	TermKey *tk;
};

/******************************************************************************
 * Class KbdStdin                                                             *
 ******************************************************************************/

KbdStdin::KbdStdin(int, int fd_stdout) : m_data(new KbdStdin::Data()) {
	// Instantiate a new termkey instance
	m_data->tk = termkey_new(0, TERMKEY_FLAG_CTRLC);
	termkey_set_waittime(m_data->tk, 1);

	// Copy the data
	m_data->fd_stdout = fd_stdout;

	// Go to alternative buffer, clear screen and hide cursor
	dprintf(m_data->fd_stdout, "\e[?1049h\e[2J\e[?25l");
}

KbdStdin::~KbdStdin() {
	// Show cursor and go to original buffer
	dprintf(m_data->fd_stdout, "\e[?25h\e[?1049l");

	// Destroy the termkey instance
	if (m_data->tk) {
		termkey_destroy(m_data->tk);
	}
}

int KbdStdin::event_fd() const { return termkey_get_fd(m_data->tk); }

EventSource::PollMode KbdStdin::event_fd_poll_mode() const {
	return EventSource::PollIn;
}

static bool termkey_key_to_event(TermKeyKey &key, Event &event) {
	// Mark this as key input
	event.type = Event::Type::KEY_INPUT;

	Event::Keyboard &data = event.data.keybd;
	data.unichar = 0;
	data.key = Event::Key::NONE;
	data.shift = key.modifiers & TERMKEY_KEYMOD_SHIFT;
	data.ctrl = key.modifiers & TERMKEY_KEYMOD_CTRL;
	data.alt = key.modifiers & TERMKEY_KEYMOD_ALT;

	switch (key.type) {
		case TERMKEY_TYPE_UNICODE: {
			data.unichar = key.code.codepoint;
			return true;
		}
#define TRANSLATE_KEY(TK_KEY, INK_KEY)  \
	case TK_KEY:                        \
		data.key = Event::Key::INK_KEY; \
		return true;
		case TERMKEY_TYPE_KEYSYM:
			switch (key.code.sym) {
				TRANSLATE_KEY(TERMKEY_SYM_BACKSPACE, BACKSPACE)
				TRANSLATE_KEY(TERMKEY_SYM_TAB, TAB)
				TRANSLATE_KEY(TERMKEY_SYM_ENTER, ENTER)
				TRANSLATE_KEY(TERMKEY_SYM_ESCAPE, ESCAPE)
				TRANSLATE_KEY(TERMKEY_SYM_DEL, DEL)
				TRANSLATE_KEY(TERMKEY_SYM_UP, UP)
				TRANSLATE_KEY(TERMKEY_SYM_DOWN, DOWN)
				TRANSLATE_KEY(TERMKEY_SYM_LEFT, LEFT)
				TRANSLATE_KEY(TERMKEY_SYM_RIGHT, RIGHT)
				TRANSLATE_KEY(TERMKEY_SYM_INSERT, INS)
				TRANSLATE_KEY(TERMKEY_SYM_PAGEUP, PAGE_UP)
				TRANSLATE_KEY(TERMKEY_SYM_PAGEDOWN, PAGE_DOWN)
				TRANSLATE_KEY(TERMKEY_SYM_HOME, HOME)
				TRANSLATE_KEY(TERMKEY_SYM_END, END)
				TRANSLATE_KEY(TERMKEY_SYM_KP0, KP_0)
				TRANSLATE_KEY(TERMKEY_SYM_KP1, KP_1)
				TRANSLATE_KEY(TERMKEY_SYM_KP2, KP_2)
				TRANSLATE_KEY(TERMKEY_SYM_KP3, KP_3)
				TRANSLATE_KEY(TERMKEY_SYM_KP4, KP_5)
				TRANSLATE_KEY(TERMKEY_SYM_KP5, KP_5)
				TRANSLATE_KEY(TERMKEY_SYM_KP6, KP_6)
				TRANSLATE_KEY(TERMKEY_SYM_KP7, KP_7)
				TRANSLATE_KEY(TERMKEY_SYM_KP8, KP_8)
				TRANSLATE_KEY(TERMKEY_SYM_KP9, KP_9)
				TRANSLATE_KEY(TERMKEY_SYM_KPENTER, KP_ENTER)
				TRANSLATE_KEY(TERMKEY_SYM_KPPLUS, KP_PLUS)
				TRANSLATE_KEY(TERMKEY_SYM_KPMINUS, KP_MINUS)
				TRANSLATE_KEY(TERMKEY_SYM_KPMULT, KP_MULT)
				TRANSLATE_KEY(TERMKEY_SYM_KPDIV, KP_DIVIDE)
				TRANSLATE_KEY(TERMKEY_SYM_KPCOMMA, KP_COMMA)
				TRANSLATE_KEY(TERMKEY_SYM_KPPERIOD, KP_PERIOD)
				TRANSLATE_KEY(TERMKEY_SYM_KPEQUALS, KP_EQUAL)
				default:
					break;
			}
#undef TRANSLATE_KEY
		default:
			break;
	}
	return false;
}

bool KbdStdin::event_get(EventSource::PollMode mode, Event &event) {

	if (mode == EventSource::PollIn) {
		TermKeyKey key;
		TermKeyResult ret = termkey_waitkey(m_data->tk, &key);
		switch (ret) {
			case TERMKEY_RES_KEY:
				if (termkey_key_to_event(key, event)) {
					return true;
				}
				break;
			default:
				return false;
		}
	}
}

}  // namespace inktty
