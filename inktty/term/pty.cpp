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

/*
 * This file is loosely based on the FBPAD FRAMEBUFFER VIRTUAL TERMINAL by
 * Ali Gholami Rudi. The original copyright notice and permission are replicated
 * below.
 *
 * ----------------------------------------------------------------------------
 *
 * Copyright (C) 2009-2018 Ali Gholami Rudi <ali at rudi dot ir>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 */

#include <cerrno>
#include <system_error>

#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <inktty/term/pty.hpp>

// Import the environment variables from the C library
extern char **environ;

namespace inktty {

/******************************************************************************
 * Helper functions                                                           *
 ******************************************************************************/

#define THROW_ON_ERR(CMD)                                           \
	do {                                                            \
		if ((CMD) < 0) {                                            \
			throw std::system_error(errno, std::system_category()); \
		}                                                           \
	} while (0)

static void allocate_pty(int *pty_master, int *pty_slave) {
	int unlock = 0, ptyno = 0;

	THROW_ON_ERR(*pty_master = open("/dev/ptmx", O_RDWR));
	THROW_ON_ERR(ioctl(*pty_master, TIOCSPTLCK, &unlock));
	THROW_ON_ERR(ioctl(*pty_master, TIOCGPTN, &ptyno));

	const std::string filename{"/dev/pts/" + std::to_string(ptyno)};
	THROW_ON_ERR(*pty_slave = open(filename.c_str(), O_RDWR | O_NOCTTY));
}

static void attach_to_pty(int pty_slave, unsigned int rows, unsigned int cols) {
	// Start a new process group
	setsid();

	// Control the terminal
	THROW_ON_ERR(ioctl(pty_slave, TIOCSCTTY, NULL));

	// Set the terminal size
	struct winsize winp;
	winp.ws_row = rows;
	winp.ws_col = cols;
	winp.ws_xpixel = 0;
	winp.ws_ypixel = 0;
	THROW_ON_ERR(ioctl(pty_slave, TIOCSWINSZ, &winp));

	// Attach the default input/output streams to the terminal
	dup2(pty_slave, 0);
	dup2(pty_slave, 1);
	dup2(pty_slave, 2);
	if (pty_slave > 2) {
		close(pty_slave);  // Free all fd's above 2
	}
}

static int spawn_child_in_pty(int pty_master, int pty_slave, unsigned int rows,
                              unsigned int cols,
                              const std::vector<std::string> &args,
                              const char *term) {
	// Push all arguments onto a list of char pointers
	std::vector<char *> argv, envp;
	for (size_t i = 0; i < args.size(); i++) {
		argv.push_back(const_cast<char *>(args[i].c_str()));
	}
	argv.push_back(nullptr);

	// Push all environment variables onto a list
	const std::string term_prefix("TERM=");
	std::string env_term = term_prefix + term;
	envp.push_back(const_cast<char *>(env_term.c_str()));
	for (size_t i = 0; environ[i]; i++) {
		if (!term_prefix.compare(0, 5, environ[i])) {
			continue; /* Skip environment variables starting with TERM= */
		}
		envp.push_back(environ[i]);
	}
	envp.push_back(nullptr);

	// Spawn a new child process
	int pid;
	THROW_ON_ERR(pid = fork());
	if (pid) {
		// This is the parent process, return with the child PID
		close(pty_slave);
		THROW_ON_ERR(fcntl(pty_master, F_SETFD,
		                   fcntl(pty_master, F_GETFD) | FD_CLOEXEC));
		THROW_ON_ERR(fcntl(pty_master, F_SETFL,
		                   fcntl(pty_master, F_GETFL) | O_NONBLOCK));
		return pid;
	}

	// Attach to the pty
	attach_to_pty(pty_slave, rows, cols);
	execve(args[0].c_str(), &argv[0], &envp[0]);
	exit(1);  // Should never return from execve(); if we do, there is an error
}

/******************************************************************************
 * Class PTY                                                                  *
 ******************************************************************************/

const char *PTY::DEFAULT_TERM = "xterm-256color";

PTY::PTY(unsigned int rows, unsigned int cols,
         const std::vector<std::string> &args, const char *term)
    : m_master_fd(-1), m_slave_fd(-1), m_child_pid(-1) {
	// We need at least the program name as an argument
	if (args.size() == 0) {
		throw std::runtime_error("Invalid number of arguments.");
	}

	// Allocate a new PTY
	allocate_pty(&m_master_fd, &m_slave_fd);

	// Spawn the child process
	m_child_pid =
	    spawn_child_in_pty(m_master_fd, m_slave_fd, rows, cols, args, term);
}

PTY::~PTY() {
	// Nicely ask the child process to terminate and wait
	if (m_child_pid >= 0 && kill(m_child_pid, SIGTERM) == 0) {
		int wstatus;
		waitpid(m_child_pid, &wstatus, 0);
	}

	// Close the master PTY
	if (m_master_fd >= 0) {
		close(m_master_fd);
	}
}

}  // namespace inktty
