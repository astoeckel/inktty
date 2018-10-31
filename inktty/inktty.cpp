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

#include <inktty/inktty.hpp>
#include <inktty/gfx/font.hpp>
#include <inktty/gfx/matrix.hpp>
#include <inktty/term/pty.hpp>

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
	PTY m_pty;

public:
	Impl(const std::vector<EventSource *> &event_sources, Display &display)
	    : m_event_sources(event_sources),
	      m_display(display),
	      m_font("/usr/share/fonts/dejavu/DejaVuSansMono.ttf", 96),
	      m_matrix(m_font, m_display),
	      m_pty(m_matrix.rows(), m_matrix.cols(), {"/usr/bin/bash"}) {
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
						std::cout << "Recevied event: NONE" << std::endl;
						break;
					case Event::Type::KEYBD_KEY_DOWN:
						std::cout << "Recevied event: KEYBD_KEY_DOWN"
						          << std::endl;
						break;
					case Event::Type::KEYBD_KEY_UP:
						std::cout << "Recevied event: KEYBD_KEY_UP"
						          << std::endl;
						break;
					case Event::Type::KEYBD_KEY_PRESS:
						std::cout << "Recevied event: KEYBD_KEY_PRESS"
						          << std::endl;
						break;
					case Event::Type::MOUSE_BTN_DOWN:
						std::cout << "Recevied event: MOUSE_BTN_DOWN"
						          << std::endl;
						break;
					case Event::Type::MOUSE_BTN_UP:
						std::cout << "Recevied event: MOUSE_BTN_UP"
						          << std::endl;
						break;
					case Event::Type::MOUSE_MOVE:
						std::cout << "Recevied event: MOUSE_MOVE" << std::endl;
						break;
					case Event::Type::MOUSE_CLICK:
						std::cout << "Recevied event: MOUSE_CLICK" << std::endl;
						break;
					case Event::Type::QUIT:
						std::cout << "Recevied event: QUIT" << std::endl;
						done = true;
						break;
					case Event::Type::RESIZE:
						std::cout << "Recevied event: RESIZE" << std::endl;
						break;
					case Event::Type::CHILD_OUTPUT:
						std::cout << "Recevied event: CHILD_OUTPUT"
						          << std::endl;
						break;
					case Event::Type::CHILD_INPUT:
						std::cout << "Recevied event: CHILD_INPUT" << std::endl;
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
