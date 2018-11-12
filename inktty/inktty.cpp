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

#include <inktty/gfx/font.hpp>
#include <inktty/gfx/matrix_renderer.hpp>
#include <inktty/inktty.hpp>
#include <inktty/term/pty.hpp>
#include <inktty/term/vt100.hpp>
#include <inktty/term/matrix.hpp>
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
	std::vector<EventSource *> m_event_sources;
	Display &m_display;
	Font m_font;
	Matrix m_matrix;
	MatrixRenderer m_matrix_renderer;
	PTY m_pty;
	VT100 m_vt100;

public:
	Impl(const std::vector<EventSource *> &event_sources, Display &display)
	    : m_event_sources(event_sources),
	      m_display(display),
	      m_font("/usr/share/fonts/dejavu/DejaVuSansMono.ttf", 96),
	      m_matrix(),
	      m_matrix_renderer(m_font, m_display, m_matrix, 10 * 64),
	      m_pty(m_matrix.size().y, m_matrix.size().x, {"/usr/bin/bash"}),
	      m_vt100(m_matrix) {
		m_event_sources.push_back(&m_pty);
	}

	~Impl() {}

	void run() {
		int evsrc = -1;
		bool done = false;
		while (!done) {
			Event event;
			if ((evsrc = Event::wait(m_event_sources, event, evsrc)) >= 0) {
				switch (event.type) {
					case Event::Type::NONE:
						break;
					case Event::Type::KEYBD_KEY_DOWN: {
						const Event::Keyboard &keybd = event.data.keybd;
						uint8_t buf[8];
						size_t buf_ptr = 0;

						/* Handle control characters */
						if (keybd.alt) {
							buf[buf_ptr++] = 0x1B;
						}
						if (keybd.ctrl && keybd.codepoint >= 'a' && keybd.codepoint <= 'z') {
							buf[buf_ptr++] = keybd.codepoint - 'a' + 1;
						} else if (keybd.is_char) {
							buf_ptr += UTF8Encoder::unicode_to_utf8(keybd.codepoint, (char*)buf + buf_ptr);
						} else if (keybd.scancode == 40 || keybd.scancode == 88) {
							buf[buf_ptr++] = '\n';
						} else if (keybd.scancode == 82) {
							buf[buf_ptr++] = 0x1B;
							buf[buf_ptr++] = 0x5B;
							buf[buf_ptr++] = 0x41;
						} else if (keybd.scancode == 81) {
							buf[buf_ptr++] = 0x1B;
							buf[buf_ptr++] = 0x5B;
							buf[buf_ptr++] = 0x42;
						} else if (keybd.scancode == 80) {
							buf[buf_ptr++] = 0x1B;
							buf[buf_ptr++] = 0x5B;
							buf[buf_ptr++] = 0x44;
						} else if (keybd.scancode == 79) {
							buf[buf_ptr++] = 0x1B;
							buf[buf_ptr++] = 0x5B;
							buf[buf_ptr++] = 0x43;
						}
						if (buf_ptr) {
							m_pty.write(buf, buf_ptr);
						}
						std::cout << "Key down SCANCODE: " << keybd.scancode << " CODEPOINT: " << keybd.codepoint << std::endl;
						break;
					}
					case Event::Type::KEYBD_KEY_UP:
						// Not handles right now
						break;
					case Event::Type::TEXT_INPUT:
						m_pty.write(event.data.text.buf,
						            event.data.text.buf_len);
						break;
					case Event::Type::MOUSE_BTN_DOWN:
						break;
					case Event::Type::MOUSE_BTN_UP:
						break;
					case Event::Type::MOUSE_MOVE:
						break;
					case Event::Type::MOUSE_CLICK:
						break;
					case Event::Type::QUIT:
						done = true;
						break;
					case Event::Type::RESIZE:
						break;
					case Event::Type::CHILD_OUTPUT:
						m_vt100.write(event.data.child.buf,
						              event.data.child.buf_len);
						m_matrix_renderer.draw();
						break;
				}
			}
		}
	}
};

/******************************************************************************
 * Class Inktty                                                               *
 ******************************************************************************/

Inktty::Inktty(const std::vector<EventSource *> &event_sources,
               Display &display)
    : m_impl(new Impl(event_sources, display)) {}

Inktty::~Inktty() {
	// Do nothing here; implicitly destroy m_impl
}

void Inktty::run() { m_impl->run(); }

}  // namespace inktty
