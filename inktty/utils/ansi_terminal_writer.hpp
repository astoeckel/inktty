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

/**
 * @file ansi_terminal_writer.hpp
 *
 * Utility code for writing with colour to the terminal. Adapted from the Ousía
 * Framework project.
 *
 * @author Andreas Stöckel
 */

#ifndef INKTTY_UTILS_ANSI_TERMINAL_WRITER_HPP
#define INKTTY_UTILS_ANSI_TERMINAL_WRITER_HPP

#include <cstdint>
#include <iosfwd>

namespace inktty {
/**
 * Object used to serialise a colour code to the terminal.
 */
struct TerminalStreamColor {
	bool active;
	uint8_t color;
	bool bright;
	friend std::ostream &operator<<(std::ostream &,
	                                const TerminalStreamColor &);
};

/**
 * Object used to serialise a background colour code to the terminal.
 */
struct TerminalStreamBackground {
	bool active;
	uint8_t color;
	bool bright;
	friend std::ostream &operator<<(std::ostream &,
	                                const TerminalStreamBackground &);
};

/**
 * Object used to serialise an RGB colour code to the terminal.
 */
struct TerminalStreamRGB {
	bool active;
	uint8_t r, g, b;
	bool background;

	friend std::ostream &operator<<(std::ostream &, const TerminalStreamRGB &);
};

/**
 * Object to serialise a "Bright" command to the terminal.
 */
struct TerminalStreamBright {
	bool active;
	friend std::ostream &operator<<(std::ostream &,
	                                const TerminalStreamBright &);
};

/**
 * Object used to serialise an "Italic" command to the terminal.
 */
struct TerminalStreamItalic {
	bool active;
	friend std::ostream &operator<<(std::ostream &,
	                                const TerminalStreamItalic &);
};

/**
 * Object used to serialise an "Underline" command to the terminal.
 */
struct TerminalStreamUnderline {
	bool active;
	friend std::ostream &operator<<(std::ostream &,
	                                const TerminalStreamUnderline &);
};

/**
 * Object used to serialise an "Underline" command to the terminal.
 */
struct TerminalStreamReset {
	bool active;
	friend std::ostream &operator<<(std::ostream &,
	                                const TerminalStreamReset &);
};

/**
 * Used by web2cpp to display coloured output on terminal emulators.
 */
class Terminal {
private:
	bool m_use_color;

public:
	/**
	 * ANSI color code for black.
	 */
	static constexpr uint8_t BLACK = 30;

	/**
	 * ANSI color code for red.
	 */
	static constexpr uint8_t RED = 31;

	/**
	 * ANSI color code for green.
	 */
	static constexpr uint8_t GREEN = 32;

	/**
	 * ANSI color code for yellow.
	 */
	static constexpr uint8_t YELLOW = 33;

	/**
	 * ANSI color code for blue.
	 */
	static constexpr uint8_t BLUE = 34;

	/**
	 * ANSI color code for magenta.
	 */
	static constexpr uint8_t MAGENTA = 35;

	/**
	 * ANSI color code for cyan.
	 */
	static constexpr uint8_t CYAN = 36;

	/**
	 * ANSI color code for white.
	 */
	static constexpr uint8_t WHITE = 37;

	/**
	 * Creates a new instance of the Terminal class.
	 *
	 * @param use_color specifies whether color codes should be generated.
	 */
	Terminal(bool use_color) : m_use_color(use_color) {}

	/**
	 * Returns a control token for switching to the given color.
	 *
	 * @param color is the color the terminal should switch to.
	 * @param bright specifies whether the terminal should switch to the bright
	 * mode.
	 * @return a control token to be included in the output stream.
	 */
	TerminalStreamColor color(uint8_t color, bool bright = true) const {
		return TerminalStreamColor{ m_use_color, color, bright };
	}

	/**
	 * Returns a control token for switching the background to the given color.
	 *
	 * @param color is the background color the terminal should switch to.
	 * @param bright indicates whether the background colour should switch to
	 * the bright mode.
	 * @return a control token to be included in the output stream.
	 */
	TerminalStreamBackground background(uint8_t color, bool bright = false) const {
		return TerminalStreamBackground{ m_use_color, color, bright };
	}

	/**
	 * Sets an RGB color.
	 */
	TerminalStreamRGB rgb(uint8_t r, uint8_t g, uint8_t b,
	                      bool background) const {
		return TerminalStreamRGB{ m_use_color, r, g, b, background };
	}

	/**
	 * Returns a control token for switching to the bright mode.
	 *
	 * @return a control token to be included in the output stream.
	 */
	TerminalStreamBright bright() const {
		return TerminalStreamBright{ m_use_color };
	}

	/**
	 * Makes the text italic.
	 *
	 * @return a control token to be included in the output stream.
	 */
	TerminalStreamItalic italic() const {
		return TerminalStreamItalic{ m_use_color };
	}

	/**
	 * Underlines the text.
	 *
	 * @return a control token to be included in the output stream.
	 */
	TerminalStreamUnderline underline() const {
		return TerminalStreamUnderline{ m_use_color };
	}

	/**
	 * Returns a control token for switching to the default mode.
	 *
	 * @return a control token to be included in the output stream.
	 */
	TerminalStreamReset reset() const {
		return TerminalStreamReset{ m_use_color };
	}
};
}

#endif /* INKTTY_UTILS_ANSI_TERMINAL_WRITER_HPP */
