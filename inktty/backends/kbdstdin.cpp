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

#include <inktty/backends/kbdstdin.hpp>

namespace inktty {

/******************************************************************************
 * Class KbdStdin::Data                                                       *
 ******************************************************************************/

struct KbdStdin::Data {
	int fd_stdin;
	int fd_stdout;
	int fd_log;
	struct termios termios;
};

/******************************************************************************
 * Class KbdStdin                                                             *
 ******************************************************************************/

KbdStdin::KbdStdin(int fd_stdin, int fd_stdout) : m_data(new KbdStdin::Data()) {
	// Copy the data
	m_data->fd_stdin = fd_stdin;
	m_data->fd_stdout = fd_stdout;

	// Copy the old termios attributes
	if (tcgetattr(m_data->fd_stdin, &m_data->termios) != 0) {
		throw std::system_error(errno, std::system_category());
	}

	// Set the new termios attributes
	struct termios new_termios = m_data->termios;
	new_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
	if (tcsetattr(m_data->fd_stdin, TCSAFLUSH, &new_termios) != 0) {
		throw std::system_error(errno, std::system_category());
	}

	// Go to alternative buffer, clear screen and hide cursor
	dprintf(m_data->fd_stdout, "\e[?1049h\e[2J\e[?25l");

	//m_data->fd_log = open("log.txt", O_CREAT | O_WRONLY | O_APPEND, 0777);
	m_data->fd_log = 1;
}

KbdStdin::~KbdStdin() {
	// Show cursor and go to original buffer
	dprintf(m_data->fd_stdout, "\e[?25h\e[?1049l");
	tcsetattr(m_data->fd_stdin, TCSAFLUSH, &m_data->termios);
	//close(m_data->fd_log);
}

int KbdStdin::event_fd() const { return m_data->fd_stdin; }

EventSource::PollMode KbdStdin::event_fd_poll_mode() const {
	return EventSource::PollIn;
}

bool KbdStdin::event_get(EventSource::PollMode mode, Event &event) {
	if (mode == EventSource::PollIn) {
		// Mark the event as a "child output" data event
		event.type = Event::Type::TEXT_INPUT;

		// Write the event-type specific data
		Event::Text &data = event.data.text;
		ssize_t n_read = read(m_data->fd_stdin, &data.buf, Event::BUF_SIZE - 1);
		if (n_read >= 0) {
			data.buf_len = n_read;
			data.buf[data.buf_len] = '\0';
			char *s = (char*)data.buf;
			while (*s) {
				dprintf(m_data->fd_log, "%02X ", (int)(*s++));
			}
			dprintf(m_data->fd_log, "\n");
			return true;
		}
	}
	// We've run into an error condition
	close(m_data->fd_stdin);
	m_data->fd_stdin = -1;
	return false;
}

}  // namespace inktty
