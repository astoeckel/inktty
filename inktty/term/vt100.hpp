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
 * @file ansi.hpp
 *
 * The VT100 class reads the incoming byte-stream, UTF-8 decodes it an writes
 * it to the text memory matrix.
 *
 * @author Andreas Stöckel
 */

#ifndef INKTTY_TERM_VT100_HPP
#define INKTTY_TERM_VT100_HPP

#include <cstddef>
#include <cstdint>
#include <memory>

#include <inktty/term/matrix.hpp>

namespace inktty {

class VT100 {
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;

public:
	VT100(Matrix &matrix);
	~VT100();

	void reset();
	void write(uint8_t *in_buf, unsigned int in_buf_len);
};

}  // namespace inktty

#endif /* INKTTY_TERM_VT100_HPP */
