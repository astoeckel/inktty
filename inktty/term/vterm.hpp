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
 * @file vterm.hpp
 *
 * The VTerm class reads the incoming byte-stream and writes it to a memory
 * matrix.
 *
 * @author Andreas Stöckel
 */

#ifndef INKTTY_TERM_VTERM_HPP
#define INKTTY_TERM_VTERM_HPP

#include <cstddef>
#include <cstdint>
#include <memory>

#include <inktty/term/events.hpp>
#include <inktty/term/matrix.hpp>

namespace inktty {

class VTerm {
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;

public:
	VTerm(Matrix &matrix);
	~VTerm();

	/**
	 * Resets the terminal to its initial state, resets all memory contents.
	 */
	void reset();

	/**
	 * Sends a keypress to the terminal. The encoded keypress is written to an
	 * internal buffer that can be accessed using send_to_pty().
	 */
	void send_key(Event::Key key, bool shift, bool ctrl, bool alt);

	void send_char(uint32_t unichar, bool shift, bool ctrl, bool alt);

	void receive_from_pty(const uint8_t *buf, size_t buf_len);

	size_t send_to_pty(uint8_t *buf, size_t buf_len);
};

}  // namespace inktty

#endif /* INKTTY_TERM_VTERM_HPP */
