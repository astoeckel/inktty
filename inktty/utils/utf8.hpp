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

#ifndef INKTTY_UTILS_UTF8
#define INKTTY_UTILS_UTF8

#include <cstdint>

namespace inktty {

class UTF8Decoder {
private:
	uint32_t m_cp;
	uint32_t m_cp_buf[4];
	unsigned int m_cp_buf_cur;
	unsigned int m_n_continuation_bytes;

public:
	struct Status {
		uint32_t codepoint;
		bool valid: 1;
		bool replaces_last: 1;
		bool error: 1;

		static Status Continue() {
			return Status{0, false, false, false};
		}

		static Status Codepoint(uint32_t codepoint, bool replaces_last=false) {
			return Status{codepoint, true,  replaces_last, false};
		}

		static Status Error() {
			return Status{0, true,  false, true};
		}

		explicit operator bool() const {
			return valid;
		}
	};

	UTF8Decoder();

	Status feed(uint8_t c);
	void reset();
};

class UTF8Encoder {
public:
	static int unicode_to_utf8(uint32_t glyph, char *s);
};
}

#endif /* INKTTY_UTILS_UTF8 */
