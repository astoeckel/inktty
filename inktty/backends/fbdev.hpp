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

#ifndef INKTTY_BACKENDS_FBDEV_HPP
#define INKTTY_BACKENDS_FBDEV_HPP

#include <inktty/gfx/display.hpp>

namespace inktty {
/**
 * Implementation of the MemDisplay class writing to a Linux framebuffer device.
 * Particularly, this class implements the update functionality required for
 * EPaper displays.
 */
class FbDevDisplay : public MemoryDisplay  {
private:
	enum class Type {
		EPaper,
		Generic
	};

	/**
	 * File descriptor pointing at the opened framebuffer device.
	 */
	int m_fb_fd;

	/**
	 * Pointer at the first byte of the memory mapped region.
	 */
	uint8_t *m_buf;

	/**
	 * Pointer at the first pixel of the framebuffer.
	 */
	uint8_t *m_buf_offs;

	/**
	 * Total size of the memory mapped region.
	 */
	size_t m_buf_size;

	/**
	 * Size of one line in bytes.
	 */
	unsigned int m_stride;

	/**
	 * Layout of the colour data.
	 */
	ColorLayout m_layout;

	/**
	 * Width of the framebuffer in pixels.
	 */
	unsigned int m_width;

	/**
	 * Height of the framebuffer in pixels.
	 */
	unsigned int m_height;

	/**
	 * Flag indicating the type of this screen.
	 */
	Type m_type;

	int m_epaper_mxc_marker;

	int m_epaper_mxc_prev_marker;

	/**
	 * Used internally to commit a change to the epaper display.
	 */
	void epaper_mxc_update(int x, int y, int w, int h, int flags);

protected:
	/* Implementation of the abstract class MemoryDisplay */
	Rect do_lock() override;
	void do_unlock(const CommitRequest *begin, const CommitRequest *end,
	               const RGBA *buf, size_t stride) override;

public:
	/**
	 * Opens the given framebuffer device file. Throws a system_error exception
	 * if opening the device fails.
	 *
	 * @param fbdev is the framebuffer device that should be opened.
	 */
	FbDevDisplay(const char *fbdev);

	/**
	 * Destroys the FbDevDisplay instance.
	 */
	~FbDevDisplay();
};
}  // namespace inktty

#endif /* INKTTY_BACKENDS_FBDEV_HPP */
