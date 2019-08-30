/*
 *  inktty -- Terminal emulator optimized for epaper displays
 *  Copyright (C) 2019  Andreas Stöckel
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

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "../inktty/fontdata/font_8x16.c"
#include "../inktty/fontdata/font_8x16.h"
#include "../lib/mxcfb.h"

/* This is a small program used to test the various update modes of the MXCFB
 * framebuffer driver for eink displays. */

// Framebuffer device to use
static const char *fbdev = "/dev/fb0";

struct RGBA {
	uint8_t r, g, b, a;
};

/**
 * Specifies the display color layout.
 */
struct ColorLayout {
	/**
	 * Bits per pixel.
	 */
	uint8_t bpp;

	/**
	 * Left/right shift per component to convert from 8 bit to the given
	 * colour.
	 */
	uint8_t rr, rl, gr, gl, br, bl;

	/**
	 * Converts the given colour to the specified colour space.
	 */
	uint32_t conv(const RGBA &c) const {
		return ((uint32_t(c.r) >> rr) << rl) | ((uint32_t(c.g) >> gr) << gl) |
		       ((uint32_t(c.b) >> br) << bl);
	}

	/**
	 * Computes the bytes per pixel.
	 */
	uint8_t bypp() const { return (bpp + 7U) >> 3U; }
};

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

	void blit_to(const ColorLayout &layout, uint8_t *tar, size_t tar_stride,
	             int tar_w, int tar_h) const {
		const int bypp = layout.bypp();
		for (int y = 0; y < std::min(tar_h, h); y++) {
			uint8_t *ptar = tar + y * tar_stride;
			const RGBA *psrc = reinterpret_cast<RGBA *>(buf + 4 * w * y);
			for (int x = 0; x < std::min(tar_w, w); x++) {
				const uint32_t cc = layout.conv(*(psrc++));
				for (int k = 0; k < bypp; k++) {
					*(ptar++) = (cc >> (8 * k)) & 0xFF;
				}
			}
		}
	}
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
								ptar[ty * tar_stride / 4 + tx] =
								    RGBA{0, 0, 0, 255};
							}
						}
					}
				}
				psrc++;
			}
		}
	}
}

static bool mxc_update(int fb_fd, int x, int y, int w, int h, int waveform_mode,
                       int update_mode, int flags) {
	static constexpr uint32_t MARKER = 0x4a58f17c;

	struct mxcfb_update_data data;
	memset(&data, 0, sizeof(data));
	data.update_region.top = y;
	data.update_region.left = x;
	data.update_region.width = w;
	data.update_region.height = h;
	data.update_marker = MARKER;

	data.waveform_mode = waveform_mode;
	data.update_mode = update_mode;
	data.flags = flags;

	return (ioctl(fb_fd, MXCFB_SEND_UPDATE, &data) >= 0) &&
	       (ioctl(fb_fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &MARKER) >= 0);
}

int main() {
	/* Try to open the framebuffer device */
	int fb_fd = open(fbdev, O_RDWR);
	if (fb_fd < 0) {
		throw std::system_error(errno, std::system_category());
	}

	/* Make sure the frame buffer is closed when the program exits (e.g. because
	   it forks). */
	if (fcntl(fb_fd, F_SETFD, fcntl(fb_fd, F_GETFD) | FD_CLOEXEC) < 0) {
		throw std::system_error(errno, std::system_category());
	}

	/* Read the screen information */
	static struct fb_var_screeninfo vinfo;
	static struct fb_fix_screeninfo finfo;
	if ((ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) ||
	    (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0)) {
		throw std::system_error(errno, std::system_category());
	}

	/* Read the screen size */
	const int w = vinfo.xres;
	const int h = vinfo.yres;

	/* Read the color layout */
	ColorLayout layout;
	layout.bpp = vinfo.bits_per_pixel;
	layout.rr = 8U - vinfo.red.length;
	layout.gr = 8U - vinfo.green.length;
	layout.br = 8U - vinfo.blue.length;
	layout.rl = vinfo.red.offset;
	layout.gl = vinfo.green.offset;
	layout.bl = vinfo.blue.offset;

	/* Memory map the frame buffer device to memory */
	const size_t buf_size = finfo.line_length * vinfo.yres_virtual;
	uint8_t *buf = (uint8_t *)mmap(NULL, buf_size, PROT_READ | PROT_WRITE,
	                               MAP_SHARED, fb_fd, 0);
	if (!buf) {
		throw std::system_error(errno, std::system_category());
	}
	const size_t stride = finfo.line_length;
	const size_t buf_offs =
	    vinfo.xoffset * layout.bypp() + vinfo.yoffset * stride;

	/* Try to load the two test images */
	Image img_test_pg1("data/mxcfb_update_modes_pg1.png");
	Image img_test_pg2("data/mxcfb_update_modes_pg2.png");

	/* Waveforms to test */
	std::vector<int> waveform_modes{
	    WAVEFORM_MODE_INIT,      WAVEFORM_MODE_DU,  WAVEFORM_MODE_GC16,
	    WAVEFORM_MODE_GC16_FAST, WAVEFORM_MODE_A2,  WAVEFORM_MODE_GL16,
	    WAVEFORM_MODE_GL16_FAST, WAVEFORM_MODE_DU4, WAVEFORM_MODE_REAGL,
	    WAVEFORM_MODE_REAGLD,    WAVEFORM_MODE_GL4, WAVEFORM_MODE_GL16_INV,
	    WAVEFORM_MODE_AUTO};
	std::vector<std::string> waveform_modes_str{"INIT",  "DU", "GC16",
	                                            "GC16F", "A2", "GL16"};

	/* Update modes to test */
	std::vector<int> update_modes{UPDATE_MODE_PARTIAL, UPDATE_MODE_FULL};
	std::vector<std::string> update_modes_str{"PARTIAL", "FULL"};

	/* Flags to test */
	std::vector<int> flags{
	    0, EPDC_FLAG_ENABLE_INVERSION, EPDC_FLAG_FORCE_MONOCHROME,
	    EPDC_FLAG_ENABLE_INVERSION | EPDC_FLAG_FORCE_MONOCHROME};
	std::vector<std::string> flags_str{"NORMAL", "INV", "MONO", "INV+MONO"};

	for (size_t i = 0; i < waveform_modes.size(); i++) {
		for (size_t j = 0; j < update_modes.size(); j++) {
			for (size_t k = 0; k < flags.size(); k++) {
				std::string test_str = waveform_modes_str[i] + "_" +
				                       update_modes_str[j] + "_" + flags_str[k];

				// Reset the screen
				std::fill(buf, buf + buf_size, 255);
				mxc_update(fb_fd, 0, 0, w, h, WAVEFORM_MODE_INIT,
				           UPDATE_MODE_FULL, 0);

				std::this_thread::sleep_for(std::chrono::seconds(3));

				// Draw the first test image
				img_test_pg1.blit_to(layout, buf + buf_offs, stride, w, h);
				draw_text(test_str, 375, 12, buf + buf_offs, stride, w, h);
				mxc_update(fb_fd, 0, 0, w, h, WAVEFORM_MODE_GC16,
				           UPDATE_MODE_FULL, 0);

				std::this_thread::sleep_for(std::chrono::seconds(3));

				// Draw the second test image
				img_test_pg2.blit_to(layout, buf + buf_offs, stride, w, h);
				draw_text(test_str, 375, 12, buf + buf_offs, stride, w, h);
				if (!mxc_update(fb_fd, 0, 0, w, h, waveform_modes[i],
				                update_modes[j], flags[k])) {
					std::cerr << "Test " << test_str << " failed." << std::endl;
				}

				std::this_thread::sleep_for(std::chrono::seconds(3));
			}
		}
	}

	munmap(buf, buf_size);
}
