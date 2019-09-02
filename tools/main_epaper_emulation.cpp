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

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include <inktty/gfx/display.hpp>
#include <inktty/gfx/epaper_emulation.cpp>
#include <inktty/utils/color.cpp>

#define STB_IMAGE_IMPLEMENTATION
#include <lib/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <lib/stb_image_write.h>

#include <inktty/fontdata/font_8x16.c>

using namespace inktty;

using OutputOp = UpdateMode::OutputOp;
using MaskOp = UpdateMode::MaskOp;

const char *output_op_strs[] = {"IDENTITY", "FORCE_MONO", "INVERT",
                                "FORCE_MONO+INVERT", "WHITE"};

const char *mask_op_strs[] = {"FULL", "SOURCE_MONO", "TARGET_MONO",
                              "SOURCE_AND_TARGET_MONO", "PARTIAL"};

struct Image {
	int w, h, n;
	uint8_t *buf;

	Image(const char *filename) : w(0), h(0), n(0), buf(nullptr) {
		buf = stbi_load(filename, &w, &h, &n, 4);
		if (!buf) {
			throw std::runtime_error(std::string("Error while loading ") +
			                         filename);
		}
	}

	~Image() { stbi_image_free(buf); }
};

static void draw_text(const std::string &str, int tar_x, int tar_y,
                      uint8_t *tar, size_t tar_stride, int tar_w, int tar_h) {

	for (size_t i = 0; i < str.size(); i++) {
		// Load the source and target pointer
		const uint8_t *psrc = fontdata_8x16 + 16 * str[i];
		RGBA *ptar = reinterpret_cast<RGBA *>(tar);

		// Render the glyph
		for (int sy = 0; sy < 16; sy += 1) {
			for (int sx_ = 0; sx_ < 8; sx_ += 8) {
				for (int sxo = 0; sxo < 8; sxo += 1) {
					// Compute the actual source x
					const int sx = sx_ + 7 - sxo;

					for (int kx = 0; kx < 2; kx++) {
						for (int ky = 0; ky < 2; ky++) {
							// Compute the target cordinates
							int tx = 2 * sx + tar_x + 2 * 8 * i + kx;
							int ty = 2 * sy + tar_y + ky;
							if (tx < 0 || tx >= (int)tar_w || ty < 0 ||
							    ty >= (int)tar_h) {
								continue;
							}

							// Set the pixel value
							if ((*psrc) & (1 << sxo)) {
								RGBA &tar = ptar[ty * tar_stride / 4 + tx];
								tar.r = ~tar.r;
								tar.g = ~tar.g;
								tar.b = ~tar.b;
							}
						}
					}
				}
				psrc++;
			}
		}
	}
}

int main() {
	/* Read the color layout */
	ColorLayout layout;
	layout.bpp = 32U;
	layout.rr = 0U;
	layout.gr = 0U;
	layout.br = 0U;
	layout.ar = 0U;
	layout.rl = 0U;
	layout.gl = 8U;
	layout.bl = 16U;
	layout.al = 24U;

	/* Try to load the test images */
	Image img_test_pg1("data/mxcfb_update_modes_pg1.png");
	Image img_test_pg2("data/mxcfb_update_modes_pg2.png");

	/* Reserve target memory */
	size_t w = img_test_pg1.w;
	size_t h = img_test_pg1.h;
	size_t stride = 4 * w;
	printf("%ld %ld %ld\n", w, h, stride);

	for (int output_op = 0; output_op <= 4; output_op++) {
		for (int mask_op = 0; mask_op <= 4; mask_op++) {
			// Skip (visually) redundant combinations
			if (output_op & UpdateMode::ForceMono) {
				if (mask_op & UpdateMode::SourceMono) {
					continue;
				}
			}
			if ((mask_op & UpdateMode::Partial) && !(output_op & UpdateMode::White)) {
				continue;
			}

			uint8_t *buf = (uint8_t *)malloc(h * stride);

			// Write the first page
			memset(buf, 255, h * stride);
			EPaperEmulation::update(buf, stride, layout,
			                        reinterpret_cast<RGBA *>(img_test_pg1.buf),
			                        stride, 0, 0, w, h, UpdateMode());

			// Write the second page
			UpdateMode mode;
			mode.output_op = OutputOp(output_op);
			mode.mask_op = MaskOp(mask_op);
			EPaperEmulation::update(buf, stride, layout,
			                        reinterpret_cast<RGBA *>(img_test_pg2.buf),
			                        stride, 0, 0, w, h, mode);

			std::string test_str = std::string(output_op_strs[output_op]) +
			                       "_" + mask_op_strs[mask_op];
			draw_text(test_str, 375, 12, buf, stride, w, h);

			char filename[255];
			snprintf(filename, 255, "epaper_emulation_%d_%d.png", output_op,
			         mask_op);
			stbi_write_png(filename, w, h, 4, buf, stride);

			free(buf);
		}
	}
}
