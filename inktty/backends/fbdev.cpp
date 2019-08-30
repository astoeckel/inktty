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

/*
 * This file is loosely based on the FBPAD FRAMEBUFFER VIRTUAL TERMINAL by
 * Ali Gholami Rudi. The original copyright notice and permission are replicated
 * below.
 *
 * ----------------------------------------------------------------------------
 *
 * Copyright (C) 2009-2018 Ali Gholami Rudi <ali at rudi dot ir>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 */

#include <cerrno>
#include <cstring>
#include <string>
#include <system_error>

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <mxcfb.h>

#include <inktty/backends/fbdev.hpp>
#include <inktty/utils/logger.hpp>

namespace inktty {

/******************************************************************************
 * Class FbDevDisplay                                                         *
 ******************************************************************************/

FbDevDisplay::FbDevDisplay(const char *fbdev) {
	/* Try to open the framebuffer device */
	m_fb_fd = open(fbdev, O_RDWR);
	if (m_fb_fd < 0) {
		throw std::system_error(errno, std::system_category());
	}

	/* Make sure the frame buffer is closed when the program exits (e.g. because
	   it forks). */
	if (fcntl(m_fb_fd, F_SETFD, fcntl(m_fb_fd, F_GETFD) | FD_CLOEXEC) < 0) {
		throw std::system_error(errno, std::system_category());
	}

	/* Read the screen information */
	static struct fb_var_screeninfo vinfo;
	static struct fb_fix_screeninfo finfo;
	if ((ioctl(m_fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) ||
	    (ioctl(m_fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0)) {
		throw std::system_error(errno, std::system_category());
	}

	/* Read the screen id */
	const std::string id(finfo.id, strnlen(finfo.id, sizeof(finfo.id)));

	/* Read the screen size */
	m_width = vinfo.xres;
	m_height = vinfo.yres;

	/* Read the color layout */
	m_layout.bpp = vinfo.bits_per_pixel;
	m_layout.rr = 8U - vinfo.red.length;
	m_layout.gr = 8U - vinfo.green.length;
	m_layout.br = 8U - vinfo.blue.length;
	m_layout.rl = vinfo.red.offset;
	m_layout.gl = vinfo.green.offset;
	m_layout.bl = vinfo.blue.offset;

	/* Print some information */
	global_logger().info() << "Opened \"" << fbdev << "\": \"" << id << "\" ("
	                       << m_width << 'x' << m_height << '@'
	                       << int(m_layout.bpp) << ")";

	/* Memory map the frame buffer device to memory */
	m_buf_size = finfo.line_length * vinfo.yres_virtual;
	m_buf = (uint8_t *)mmap(NULL, m_buf_size, PROT_READ | PROT_WRITE,
	                        MAP_SHARED, m_fb_fd, 0);
	if (!m_buf) {
		throw std::system_error(errno, std::system_category());
	}
	m_stride = finfo.line_length;
	m_buf_offs =
	    m_buf + vinfo.xoffset * m_layout.bypp() + vinfo.yoffset * m_stride;
}

FbDevDisplay::~FbDevDisplay() {
	munmap(m_buf, m_buf_size);
	close(m_fb_fd);
}

Rect FbDevDisplay::do_lock() { return Rect(0, 0, m_width, m_height); }

static void kobo_eink_update_partial(int fb_fd, int mono, int left, int top,
                                     int width, int height) {
	struct mxcfb_update_data region;
	static int prev_marker = 0;
	static int marker = 1;
	static int mono_no = 0;

	marker++;
	if (marker > 1024) {
		marker = 1;
	}
	if (prev_marker) {
		ioctl(fb_fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE, prev_marker);
	}

	region.update_marker = marker; /* Marker used when waiting for completion */
	region.update_region.top = top;
	region.update_region.left = left;
	region.update_region.width = width;
	region.update_region.height = height;
	// 1 is quite good and fast in bw
	// 2 doesn't work
	// 3 doesn't work
	// 4 works

	/* maybe switch to waveform 4 after having ensured stable bw?.. */

	region.update_mode = UPDATE_MODE_PARTIAL;
	// region.update_mode = UPDATE_MODE_FULL;
	region.temp = TEMP_USE_AMBIENT;
	if (mono) {
		if (mono_no == 0)
			region.waveform_mode = WAVEFORM_MODE_AUTO;
		else
			region.waveform_mode = 4;
		region.flags = EPDC_FLAG_FORCE_MONOCHROME;
		mono_no++;
	} else {
		region.waveform_mode = WAVEFORM_MODE_AUTO;
		region.flags = 0;
		mono_no = 0;
	}
	ioctl(fb_fd, MXCFB_SEND_UPDATE, &region);
	prev_marker = marker;
}

void FbDevDisplay::do_unlock(const CommitRequest *begin,
                             const CommitRequest *end, const RGBA *buf,
                             size_t stride) {
	const int bypp = m_layout.bypp();
	for (CommitRequest const *req = begin; req < end; req++) {
		const Rect r = req->r;
		const int w = r.width();
		for (int y = r.y0; y < r.y1; y++) {
			uint8_t *ptar = m_buf_offs + y * m_stride + r.x0 * bypp;
			RGBA const *psrc = buf + y * stride / sizeof(RGBA) + r.x0;
			for (int x = 0; x < w; x++) {
				const uint32_t cc = m_layout.conv(*(psrc++));
				for (int k = 0; k < bypp; k++) {
					*(ptar++) = (cc >> (8 * k)) & 0xFF;
				}
			}
		}

		kobo_eink_update_partial(m_fb_fd, 1, r.x0, r.y0, r.width(), r.height());
	}
}

}  // namespace inktty
