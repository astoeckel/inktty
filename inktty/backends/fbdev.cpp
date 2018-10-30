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

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cerrno>
#include <system_error>

#include <inktty/backends/fbdev.hpp>

namespace inktty {

/******************************************************************************
 * Class FbDevDisplay                                                         *
 ******************************************************************************/

FbDevDisplay::FbDevDisplay(const char *fbdev)
{
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

FbDevDisplay::~FbDevDisplay()
{
	munmap(m_buf, m_buf_size);
	close(m_fb_fd);
}

void FbDevDisplay::commit(int x, int y, int w, int h, CommitMode mode) {}
}  // namespace inktty
