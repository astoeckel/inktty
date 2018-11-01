/*
 *  libfoxenbitstream -- Tiny, inflexible bitstream reader
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

#include <foxen/unittest.h>
#include <inktty/utils/utf8.hpp>

using namespace inktty;

const char *ASCII_INPUT = "Hello World!\n\r";
const char *LATIN1_INPUT = "Smørrebrød\nGemütlichkeit";
const char *EMOJI_INPUT = "🤪";
const char *DENORM_INPUT = "u\xcc\x88";

void test_utf8_decoder_ascii() {
	UTF8Decoder dec;
	char const *s = ASCII_INPUT;
	while (*s) {
		UTF8Decoder::Status status = dec.feed(*s);
		EXPECT_TRUE((bool)status);
		EXPECT_TRUE(status.valid);
		EXPECT_FALSE(status.replaces_last);
		EXPECT_FALSE(status.error);
		EXPECT_EQ((uint32_t)*s, status.codepoint);
		s++;
	}
}

static void step(UTF8Decoder &dec, char const **s, uint32_t codepoint,
                 bool valid, bool replaces_last, bool error) {
	if (!**s) {
		return;
	}
	UTF8Decoder::Status status = dec.feed(**s);
	EXPECT_EQ(codepoint, status.codepoint);
	EXPECT_EQ(valid, status.valid);
	EXPECT_EQ(replaces_last, status.replaces_last);
	EXPECT_EQ(error, status.error);
	(*s)++;
}

void test_utf8_decoder_latin1() {
	UTF8Decoder dec;
	char const *s = LATIN1_INPUT;
	step(dec, &s, 'S', true, false, false);
	step(dec, &s, 'm', true, false, false);
	step(dec, &s, 0, false, false, false);
	step(dec, &s, 0xF8, true, false, false);
	step(dec, &s, 'r', true, false, false);
	step(dec, &s, 'r', true, false, false);
	step(dec, &s, 'e', true, false, false);
	step(dec, &s, 'b', true, false, false);
	step(dec, &s, 'r', true, false, false);
	step(dec, &s, 0, false, false, false);
	step(dec, &s, 0xF8, true, false, false);
	step(dec, &s, 'd', true, false, false);
	step(dec, &s, '\n', true, false, false);
	step(dec, &s, 'G', true, false, false);
	step(dec, &s, 'e', true, false, false);
	step(dec, &s, 'm', true, false, false);
	step(dec, &s, 0, false, false, false);
	step(dec, &s, 0xFC, true, false, false);
	step(dec, &s, 't', true, false, false);
	step(dec, &s, 'l', true, false, false);
	step(dec, &s, 'i', true, false, false);
	step(dec, &s, 'c', true, false, false);
	step(dec, &s, 'h', true, false, false);
	step(dec, &s, 'k', true, false, false);
	step(dec, &s, 'e', true, false, false);
	step(dec, &s, 'i', true, false, false);
	step(dec, &s, 't', true, false, false);
}

void test_utf8_decoder_emoji() {
	UTF8Decoder dec;
	char const *s = EMOJI_INPUT;
	step(dec, &s, 0, false, false, false);
	step(dec, &s, 0, false, false, false);
	step(dec, &s, 0, false, false, false);
	step(dec, &s, 0x1F92A, true, false, false);
}

void test_utf8_decoder_normalisation() {
	UTF8Decoder dec;
	char const *s = DENORM_INPUT;
	step(dec, &s, 'u', true, false, false);
	step(dec, &s, 0, false, false, false);
	step(dec, &s, 0xFC, true, true, false);
}

int main() {
	RUN(test_utf8_decoder_ascii);
	RUN(test_utf8_decoder_latin1);
	RUN(test_utf8_decoder_emoji);
	RUN(test_utf8_decoder_normalisation);
	DONE;
}
