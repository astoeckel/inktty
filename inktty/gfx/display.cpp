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

#include <inktty/gfx/display.hpp>

#include <stdio.h>

namespace inktty {

/******************************************************************************
 * Class Display                                                              *
 ******************************************************************************/

Display::~Display()
{
	// Do nothing here
}

/******************************************************************************
 * Class MemDisplay                                                           *
 ******************************************************************************/

void MemDisplay::blit(const uint8_t *mask, int stride, const RGB &fg, const RGB &bg,
	                  int x, int y, int w, int h)
{
	// Clip the rectangle to the screen bounding box
	uint8_t *poffs = this->buf();
	const int pstride = this->stride();
	const int tar_x0 = clipx(x), tar_x1 = clipx(x + w);
	const int tar_y0 = clipy(y), tar_y1 = clipy(y + h);
	const int src_x0 = tar_x0 - x;
	const int src_y0 = tar_y0 - y;

	// Convert the color to an integer corresponding to the color layout
	const ColorLayout &cl = layout();
	const int bypp = cl.bypp();

	for (int i = tar_y0, i_src = src_y0; i < tar_y1; i++, i_src++) {
		uint8_t *ptar = poffs + pstride * i + bypp * tar_x0;
		uint8_t const *psrc = mask + stride * i_src + src_x0;
		for (int j = tar_x0; j < tar_x1; j++) {
			const uint8_t a = *(psrc++);
			const uint8_t r = (uint16_t(fg.r) * a + uint16_t(bg.r) * (255U - a)) >> 8U;
			const uint8_t g = (uint16_t(fg.g) * a + uint16_t(bg.g) * (255U - a)) >> 8U;
			const uint8_t b = (uint16_t(fg.b) * a + uint16_t(bg.b) * (255U - a)) >> 8U;
			const uint32_t cc = cl.conv(RGB(r, g, b));
			for (int k = 0; k < bypp; k++) {
				if (a) {
					*(ptar++) = (cc >> (8 * k)) & 0xFF;
				} else {
					ptar++;
				}
			}
		}
	}
}

void MemDisplay::fill(const RGB &c, int x, int y, int w, int h)
{
	// Clip the rectangle to the screen bounding box
	uint8_t *poffs = buf();
	const int pstride = stride();
	const int x0 = clipx(x), x1 = clipx(x + w);
	const int y0 = clipy(y), y1 = clipy(y + h);

	// Convert the color to an integer corresponding to the color layout
	const ColorLayout &cl = layout();
	const int bypp = cl.bypp();
	const uint32_t cc = cl.conv(c);

	// Fill each line with the specified color
	// TODO: this code can be heavily optimized
	for (int i = y0; i < y1; i++) {
		uint8_t *p = poffs + pstride * i + bypp * x0;
		for (int j = x0; j < x1; j++) {
			for (int k = 0; k < bypp; k++) {
				*(p++) = (cc >> (8 * k)) & 0xFF;
			}
		}
	}
}

}  // namespace inktty
