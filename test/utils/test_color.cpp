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
#include <inktty/utils/color.hpp>

using namespace inktty;

void test_color_layout() {
	{
		ColorLayout layout;
		layout.rr = 0;
		layout.rl = 0;
		layout.gr = 0;
		layout.gl = 8;
		layout.br = 0;
		layout.bl = 16;
		layout.ar = 0;
		layout.al = 24;

		uint32_t x = layout.conv_from_rgba(RGBA{0x1A, 0x2B, 0x3C, 0x4D});
		RGBA c = layout.conv_to_rgba(x);

		EXPECT_EQ(0x1A, c.r);
		EXPECT_EQ(0x2B, c.g);
		EXPECT_EQ(0x3C, c.b);
		EXPECT_EQ(0x4D, c.a);
	}

	{
		ColorLayout layout;
		layout.rr = 1;
		layout.rl = 0;
		layout.gr = 2;
		layout.gl = 8;
		layout.br = 3;
		layout.bl = 16;
		layout.ar = 4;
		layout.al = 24;

		uint32_t x = layout.conv_from_rgba(RGBA{0x1A, 0x2B, 0x3C, 0x4D});
		RGBA c = layout.conv_to_rgba(x);

		EXPECT_EQ(0x1A, c.r);
		EXPECT_EQ(0x28, c.g);
		EXPECT_EQ(0x38, c.b);
		EXPECT_EQ(0x40, c.a);
	}
}

int main() {
	RUN(test_color_layout);
	DONE;
}
