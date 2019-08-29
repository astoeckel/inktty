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

#include <iostream>
#include <memory>

#include <stdlib.h>
#include <time.h>

#include <inktty/gfx/font.hpp>
#include <inktty/gfx/font_bitmap.hpp>
#include <inktty/gfx/font_ttf.hpp>
#include <inktty/gfx/matrix_renderer.hpp>
#include <inktty/inktty.hpp>
#include <inktty/term/matrix.hpp>
#include <inktty/term/pty.hpp>
#include <inktty/term/vterm.hpp>
#include <inktty/utils/utf8.hpp>

namespace inktty {

/******************************************************************************
 * CLASS IMPLEMENTATIONS                                                      *
 ******************************************************************************/

/******************************************************************************
 * Class Inktty::Impl                                                         *
 ******************************************************************************/

class Inktty::Impl {
private:
	const Configuration &m_config;
	std::vector<EventSource *> m_event_sources;
	Display &m_display;
	Font *m_font;
	Matrix m_matrix;
	MatrixRenderer m_matrix_renderer;
	PTY m_pty;
	VTerm m_vterm;
	int64_t m_t_last_draw;
	bool m_needs_redraw;

	static int64_t microtime() {
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
		return (int64_t(ts.tv_sec) * 1000 * 1000) +
		       (int64_t(ts.tv_nsec) / 1000);
	}

	static const char *get_shell() {
		char const *shell = getenv("SHELL");
		if (!shell) {
			return "/bin/sh";
		}
		return shell;
	}

public:
	Impl(const Configuration &config,
	     const std::vector<EventSource *> &event_sources, Display &display)
	    : m_config(config),
	      m_event_sources(event_sources),
	      m_display(display),
#ifdef HAS_FREETYPE
	      m_font(new FontTTF("DejaVuSansMono.ttf", 96)),
#else
	      m_font(&FontBitmap::Font8x16),
#endif
	      m_matrix(),
	      m_matrix_renderer(m_config, *m_font, m_display, m_matrix, 13 * 64, 0),
	      m_pty(m_matrix.size().y, m_matrix.size().x, {get_shell()}),
	      m_vterm(m_matrix),
	      m_t_last_draw(microtime()),
	      m_needs_redraw(false) {
		m_event_sources.push_back(&m_pty);
	}

	~Impl() {
#ifdef HAS_FREETYPE
		delete m_font;  // XXX
#endif
	}

	bool handle_event(const Event &event) {
		switch (event.type) {
			case Event::Type::NONE:
				break;
			case Event::Type::KEY_INPUT: {
				const Event::Keyboard &k = event.data.keybd;
				if (k.key != Event::Key::NONE) {
					m_vterm.send_key(k.key, k.shift, k.ctrl, k.alt);
				} else if (k.unichar) {
					m_vterm.send_char(k.unichar, k.shift, k.ctrl, k.alt);
				}
				break;
			}
			case Event::Type::TEXT_INPUT: {
				const Event::Text &t = event.data.text;
				UTF8Decoder utf8;
				for (size_t i = 0; i < t.buf_len; i++) {
					UTF8Decoder::Status res;
					if ((res = utf8.feed(t.buf[i])) && !res.error) {
						m_vterm.send_char(res.codepoint, false, false, false);
					}
				}
				break;
			}
			case Event::Type::MOUSE_BTN_DOWN:
				break;
			case Event::Type::MOUSE_BTN_UP:
				break;
			case Event::Type::MOUSE_MOVE:
				break;
			case Event::Type::MOUSE_CLICK:
				break;
			case Event::Type::QUIT:
				return true;
			case Event::Type::RESIZE:
				break;
			case Event::Type::CHILD_OUTPUT:
				m_vterm.receive_from_pty(event.data.child.buf,
				                         event.data.child.buf_len);
				m_needs_redraw = true;
				break;
		}
		return false;
	}

	void run() {
		int evsrc = -1;
		bool done = false;
		uint8_t buf[64];
		size_t buf_len = 0;
		while (!done) {
			Event event;

			// Redraw the UI in 16.667 ms intervals (corresponding to 60 Hz)
			const int64_t t = microtime();
			int timeout = -1;
			if (m_needs_redraw) {
				timeout = (16667 - (t - m_t_last_draw)) / 1000;
				if (timeout <= 0) {
					m_matrix_renderer.draw();
					m_t_last_draw = t;
					m_needs_redraw = false;
					timeout = -1;
				}
			}

			// Wait for a new event or sleep for the timeout calculated above
			evsrc = Event::wait(m_event_sources, event, evsrc, timeout);

			// If there was an event, handle the event
			if (evsrc >= 0) {
				done = handle_event(event);
			}

			// Forward the output of the terminal to the PTY
			while ((buf_len = m_vterm.send_to_pty(buf, sizeof(buf)))) {
				m_pty.write(buf, buf_len);
			}
		}
	}
};

/******************************************************************************
 * Class Inktty                                                               *
 ******************************************************************************/

Inktty::Inktty(const Configuration &config,
               const std::vector<EventSource *> &event_sources,
               Display &display)
    : m_impl(new Impl(config, event_sources, display)) {}

Inktty::~Inktty() {
	// Do nothing here; implicitly destroy m_impl
}

void Inktty::run() { m_impl->run(); }

}  // namespace inktty
