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
 * @file tty.hpp
 *
 * Provides a wrapper around the Unix pseudo-terminal. Allows to allocate a
 * pseudo-terminal and attach a child process.
 *
 * @author Andreas Stöckel
 */

#ifndef INKTTY_TERM_PTY_HPP
#define INKTTY_TERM_PTY_HPP

#include <string>
#include <vector>

#include <inktty/term/events.hpp>

namespace inktty {

/**
 * The PTY class represents a pseudo-terminal with a single attached child
 * process running inside.
 */
class PTY: public EventSource {
private:
	/**
	 * File-descriptor associated with the PTY.
	 */
	int m_master_fd;

	/**
	 * File-descriptor associated with the PTY as handed to the child process.
	 */
	int m_slave_fd;

	/**
	 * PID of the child process.
	 */
	int m_child_pid;

	/**
	 * Write buffer containing the data that was not yet transmitted to the
	 * client.
	 */
	std::vector<uint8_t> m_write_buf;

public:
	/**
	 * Default value for the TERM environment variable. Should be
	 * "xterm-256color".
	 */
	static const char *DEFAULT_TERM;

	/**
	 * Creates a new PTY instance. Sets the terminal size to the given values
	 * and spawns the program specified by "args". The launched process will
	 * inherit the environment with the additional environment variable "TERM"
	 * set to the specified value (default is "xterm-256color"). Throws a
	 * std::system_error if an error occurs.
	 *
	 *
	 * @param rows number of rows.
	 * @param cols number of cols.
	 * @param args list describing the program that should be launched. First
	 * argument is the program executable.
	 * @param term is the value for the TERM environment variable of the child
	 * process. Default value is "xterm-256color".
	 */
	PTY(unsigned int rows, unsigned int cols,
	    const std::vector<std::string> &args, const char *term = DEFAULT_TERM);

	/**
	 * Kills the child process and closes the PTY.
	 */
	~PTY() override;

	/**
	 * Informs the child process that the terminal has resized.
	 *
	 * @param rows new number of rows.
	 * @param cols new number of cols.
	 */
	void resize(unsigned int rows, unsigned int cols);

	/**
	 * Returns the file descriptor associated with the PTY.
	 */
	int fd() const { return m_master_fd; }

	/**
	 * Returns the process ID of the child process.
	 */
	int child_pid() const { return m_child_pid; }

	/**
	 * Sends data to the client via STDIN.
	 */
	void write(uint8_t *buf, size_t buf_len);

	/* Interface EventSource */

	int event_fd() const override;
	EventSource::PollMode event_fd_poll_mode() const override;
	bool event_get(EventSource::PollMode mode, Event &event) override;
};

}  // namespace inktty

#endif /* INKTTY_TERM_PTY_HPP */
