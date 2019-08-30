/*
 *  inktty -- Terminal emulator optimized for epaper displays
 *  Copyright (C) 2018, 2019  Andreas Stöckel
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

#include <ostream>

#include <inktty/utils/ansi_terminal_writer.hpp>

namespace inktty {
std::ostream &operator<<(std::ostream &os, const TerminalStreamColor &t) {
	if (t.active) {
		os << "\x1b[";
		if (t.bright) {
			os << "1;";
		}
		os << (int)t.color << "m";
	}
	return os;
}

std::ostream &operator<<(std::ostream &os, const TerminalStreamBackground &t) {
	if (t.active) {
		os << "\x1b[" << (int)(t.color + (t.bright ? 70 : 10)) << "m";
	}
	return os;
}

std::ostream &operator<<(std::ostream &os, const TerminalStreamRGB &t) {
	if (t.active) {
#if 0
	os << "\x1b[" << (t.background ? "48" : "38") << ";2;" << (int)t.r << ";"
	   << (int)t.g << ";" << (int)t.b << "m";
#else
		size_t code = 0;
		if (t.r == t.g && t.g == t.b) {
			// Issue a BW color
			if (t.r == 0) {
				code = 16;
			} else {
				code = 232 + t.r * 24 / 256;
			}
		} else {
			// Issue a color from the 6 x 6 x 6 color cube
			uint8_t offsB = t.b * 6 / 256;
			uint8_t offsG = t.g * 6 / 256;
			uint8_t offsR = t.r * 6 / 256;
			code = 16 + offsR * 36 + offsG * 6 + offsB;
		}

		// Generate the output code
		os << "\x1b[" << (t.background ? "48" : "38") << ";5;" << code << "m";
#endif
	}
	return os;
}

std::ostream &operator<<(std::ostream &os, const TerminalStreamBright &t) {
	if (t.active) {
		os << "\x1b[1m";
	}
	return os;
}

std::ostream &operator<<(std::ostream &os, const TerminalStreamItalic &t) {
	if (t.active) {
		os << "\x1b[3m";
	}
	return os;
}

std::ostream &operator<<(std::ostream &os, const TerminalStreamUnderline &t) {
	if (t.active) {
		os << "\x1b[4m";
	}
	return os;
}

std::ostream &operator<<(std::ostream &os, const TerminalStreamReset &t) {
	if (t.active) {
		os << "\x1b[0m";
	}
	return os;
}
}
