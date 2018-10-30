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
class FbDevDisplay : public MemDisplay {
private:
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

protected:
	/**
	 * Returns the color layout as read from the frame buffer.
	 */
	const ColorLayout &layout() const override { return m_layout; }

	/**
	 * Returns the pointer at the first pixel in memory.
	 */
	uint8_t *buf() const override { return m_buf_offs; }

	/**
	 * Returns the size of one line on the screen in bytes as it is stored in
	 * memory.
	 */
	unsigned int stride() const override { return m_stride; }

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

	/**
	 * Commits the specified region to the screen. While this is not necessary
	 * on standard framebuffer devices, epaper displays explicitly need to be
	 * informed about which part of the screen should be updated and how the
	 * update should be performed.
	 *
	 * @param x is the x-coordinate of the top-left corner of the region that
	 * should be updated.
	 * @param y is the y-coordinate of the top-left corner of the region that
	 * should be updated.
	 * @param w is the width of the rectangle that should be updated in pixels.
	 * @param h is the height of the rectangle that should be updated in pixels.
	 * @param mode is the commit mode that specifies how the display should be
	 * updated.
	 */
	void commit(int x, int y, int w, int h, CommitMode mode) override;

	/**
	 * Returns the width of the framebuffer device in pixels.
	 */
	unsigned int width() const override { return m_width; }

	/**
	 * Returns the height of the framebuffer device in pixels.
	 */
	unsigned int height() const override { return m_height; }
};
}  // namespace inktty

#endif /* INKTTY_BACKENDS_FBDEV_HPP */
