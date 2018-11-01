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

#include <utf8proc/utf8proc.h>
#include <inktty/utils/utf8.hpp>

namespace inktty {
/******************************************************************************
 * Class UTF8Encoder                                                          *
 ******************************************************************************/

UTF8Decoder::UTF8Decoder() { reset(); }

UTF8Decoder::Status UTF8Decoder::feed(uint8_t c) {
	const size_t N = sizeof(m_cp_buf) / sizeof(m_cp_buf[0]);
	bool replaces_last = false;
	ssize_t n = 0;

	if ((c & 0xC0) == 0x80) {  // Continuation byte
		if (m_n_continuation_bytes == 0) {
			goto error;  // We were not expecting continuation bytes
		}
		m_n_continuation_bytes--;
		m_cp |= (c & 0x3F) << (m_n_continuation_bytes * 6U);
	} else {  // Non-continuation byte
		if (m_n_continuation_bytes > 0) {
			goto error;  // We are expecting continuation bytes
		}
		// Handle the four possible UTF8 codepoint start bytes
		if ((c & 0x80) == 0x00) {
			m_cp = c;
			m_n_continuation_bytes = 0;
		} else if ((c & 0xE0) == 0xC0) {
			m_cp = (c & 0x1F) << 6;
			m_n_continuation_bytes = 1;
		} else if ((c & 0xF0) == 0xE0) {
			m_cp = (c & 0x0F) << 12;
			m_n_continuation_bytes = 2;
		} else if ((c & 0xF8) == 0xF0) {
			m_cp = (c & 0x07) << 18;
			m_n_continuation_bytes = 3;
		} else {
			goto error;
		}
	}

	// If there are continuation bytes left for this code-point, continue
	if (m_n_continuation_bytes) {
		return Status::Continue();
	}

	// Append the glyph to the glyph buffer
	if (m_cp_buf_cur == N) {
		for (size_t i = 1; i < N; i++) {
			m_cp_buf[i - 1] = m_cp_buf[i];
		}
		m_cp_buf_cur--;
	}
	m_cp_buf[m_cp_buf_cur] = m_cp;
	m_cp_buf_cur++;

	// Try to normalize the codepoints in the codepoint buffer
	n = utf8proc_normalize_utf32((utf8proc_int32_t *)m_cp_buf,
	                                           m_cp_buf_cur, UTF8PROC_COMPOSE);
	if (n < 0) {
		goto error;
	} else if (n < m_cp_buf_cur) {
		replaces_last = true;
		m_cp_buf_cur = n;
		m_cp = m_cp_buf[m_cp_buf_cur - 1];
	}
	return Status::Codepoint(m_cp, replaces_last);

error:
	m_cp = 0;
	m_cp_buf_cur = 0;
	m_n_continuation_bytes = 0;
	return Status::Error();
}

void UTF8Decoder::reset() {
	m_cp = 0;
	m_cp_buf_cur = 0;
	m_n_continuation_bytes = 0;
}

/******************************************************************************
 * Class UTF8Encoder                                                          *
 ******************************************************************************/

int UTF8Encoder::unicode_to_utf8(uint32_t glyph, char *s) {
	if (glyph < 0x80) {
		s[0] = glyph;
		return 1;
	} else if (glyph < 0x0800) {
		s[0] = 0xC0 | ((glyph >> 6) & 0x7F);
		s[1] = 0x80 | ((glyph >> 0) & 0x3F);
		return 2;
	} else if (glyph < 0x10000) {
		s[0] = 0xE0 | ((glyph >> 12) & 0x7F);
		s[1] = 0x80 | ((glyph >> 6) & 0x3F);
		s[2] = 0x80 | ((glyph >> 0) & 0x3F);
		return 3;
	} else if (glyph < 0x11FFFF) {
		s[0] = 0xF0 | ((glyph >> 18) & 0x7F);
		s[1] = 0x80 | ((glyph >> 12) & 0x3F);
		s[2] = 0x80 | ((glyph >> 6) & 0x3F);
		s[3] = 0x80 | ((glyph >> 0) & 0x3F);
		return 4;
	}
	return 0;
}
}  // namespace inktty
