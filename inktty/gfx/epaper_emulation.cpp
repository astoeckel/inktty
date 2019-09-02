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

#include <inktty/gfx/epaper_emulation.hpp>

namespace inktty {
namespace epaper_emulation {
void update(uint8_t *tar, size_t tar_stride, const ColorLayout &tar_layout,
            const RGBA *src, size_t src_stride, int x0, int y0, int x1, int y1,
            UpdateMode mode) {
	const int tar_bypp = tar_layout.bypp();
	for (int y = y0; y < y1; y++) {
		uint8_t *ptar = tar + y * tar_stride + x0 * tar_bypp;
		RGBA const *psrc = src + y * src_stride / sizeof(RGBA) + x0;
		for (int x = 0; x < x1 - x0; x++) {
			// Read the current color in the target surface, as well as the
			// source surface
			uint32_t c_tar32 = 0;
			for (int k = 0; k < tar_bypp; k++) {
				c_tar32 |= ptar[k] << (8 * k);
			}
			RGBA c_tar = tar_layout.conv_to_rgba(c_tar32);
			RGBA c_src = *(psrc++);

			// Convert all colours to 16 bit greyscale
			uint8_t g_tar = rgba_to_greyscale(c_tar);
			uint8_t g_src = rgba_to_greyscale(c_src);

			// Apply the output operation (except for "white")
			if (mode.output_op & UpdateMode::Invert) {
				g_src = 15U - g_src;
			}
			if (mode.output_op & UpdateMode::ForceMono) {
				g_src = (g_src > 7U) ? 15U : 0U;
			}

			// Compute the update mask, i.e. skip updating pixels that do not
			// match the specified operation
			bool masked = false;
			if (mode.mask_op & UpdateMode::SourceMono) {
				if (g_src != 0U && g_src != 15U) {
					masked = true;
				}
			}
			if (mode.mask_op & UpdateMode::TargetMono) {
				if (g_tar != 0U && g_tar != 15U) {
					masked = true;
				}
			}
			if (mode.mask_op & UpdateMode::Partial) {
				if (g_tar == g_src) {
					masked = true;
				}
			}

			if (mode.output_op & UpdateMode::White) {
				g_src = 15U;
			}

			// Write the processed target color back
			uint32_t cc;
			if (!masked) {
				cc = tar_layout.conv_from_rgba(greyscale_to_rgba(g_src));
			} else {
				cc = tar_layout.conv_from_rgba(greyscale_to_rgba(g_tar));
			}
			for (int k = 0; k < tar_bypp; k++) {
				*(ptar++) = (cc >> (8 * k)) & 0xFF;
			}
		}
	}
}

uint8_t rgba_to_greyscale(const RGBA &x) {
	const uint16_t r = x.r * 77;
	const uint16_t g = x.g * 151;
	const uint16_t b = x.b * 28;
	return ((r + g + b) >> 12U);
}

RGBA greyscale_to_rgba(uint8_t g) {
	const uint8_t lookup[16] = {0,   17,  34,  51,  68,  85,  102, 119,
	                            136, 153, 170, 187, 204, 221, 238, 255};
	const uint8_t x = lookup[g & 0x0F];
	return RGBA{x, x, x, 0xFF};
}
}  // namespace epaper_emulation
}  // namespace inktty

