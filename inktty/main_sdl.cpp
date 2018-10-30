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

#include <inktty/backends/sdl.hpp>

#include <inktty/gfx/font.hpp>
#include <inktty/gfx/matrix.hpp>

#include <inktty/term/pty.hpp>

#include <unistd.h>
#include <poll.h>

using namespace inktty;


int main(int argc, char *argv[])
{
	SDLBackend display(1600, 1200);
	Font font("/usr/share/fonts/dejavu/DejaVuSansMono.ttf", 96);
	Matrix matrix(font, display, 11 * 64);

	PTY pty(40, 80, {"/usr/bin/env"});

	struct pollfd ufds[1];
	ufds[0].fd = pty.fd();
	ufds[0].events = POLLIN;

	int i = 0, j = 0;
	char buf;
	while (read(pty.fd(), &buf, 1) == 1) {
		if (buf == '\n') {
			i = 0;
			j++;
			continue;
		}
		matrix.set_cell(j, i++, buf, Style{});
		if (i >= matrix.cols()) {
			i = 0;
			j++;
		}
	}

	int r =0;
	while (display.wait(1000)) {
		matrix.set_orientation(r++);
		matrix.update();

		matrix.draw();

		display.commit(0, 0, display.width(), display.height(),
			           Display::CommitMode::full);
	}

	return 0;
}
