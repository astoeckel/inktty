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

#ifndef INKTTY_GFX_DISPLAY_HPP
#define INKTTY_GFX_DISPLAY_HPP

#include <cstddef>
#include <cstdint>

#include <inktty/gfx/color.hpp>

namespace inktty {
/**
 * Abstract display class used in the rest of the application to interface with
 * the screen. Conceptually the display consists of a back-buffer and a
 * front-buffer. All draw operations only affect the back-buffer until they are
 * commited to the screen.
 */
class Display {
public:
	/**
	 * The CommitMode enum is used to specify the quality with which the screen
	 * is updated. The screen update can either be fast with low quality or slow
	 * with a high quality.
	 */
	enum class CommitMode {
		/**
		 * Perform an automatic partial update with default quality.
		 */
		partial,

		/**
		 * Perform a partial update with a forced flash-to-black.
		 */
		flash_partial,

		/**
		 * Perform an with low quality as used for UI elements.
		 */
		flash_ui,

		/**
		 * Perform a full, high-quality update.
		 */
		full,

		/**
		 * Perform a fast update with low-quality.
		 */
		fast,
	};

	virtual ~Display();

	/**
	 * Blits the given mask bitmap (i.e. an 8-bit grayscale image) to the
	 * display at location (x, y).
	 *
	 * @param mask is a pointer at a 8-bit grayscale image. Black regions in
	 * this image (with value zero) are replaced with the background colour,
	 * white regions with the foreground color. Grayscales in between are
	 * replaced with a blend between the foreground and background colour.
	 * @param stride is the width of one line in the mask image in bytes.
	 * @param fg is the foreground color that should be used.
	 * @param bg is the background color that should be used.
	 * @param x is the x-coordinate of the top-left corner of the rectangle that
	 * should be filled.
	 * @param y is the y-coordinate of the top-left corner of the rectangle that
	 * should be filled.
	 * @param w is the width of the rectangle in pixels.
	 * @param h is the height of the rectangle in pixels.
	 */
	virtual void blit(const uint8_t *mask, int stride, const RGB &fg, const RGB &bg,
	                  int x, int y, int w, int h) = 0;

	/**
	 * Fills the specified rectangle with the given colour.
	 *
	 * @param c is the colour the rectangle should be filled with.
	 * @param x is the x-coordinate of the top-left corner of the rectangle that
	 * should be filled.
	 * @param y is the y-coordinate of the top-left corner of the rectangle that
	 * should be filled.
	 */
	virtual void fill(const RGB &c, int x, int y, int w, int h) = 0;

	/**
	 * Commits the content written to the display to the screen in the given
	 * region. This function allows to specify a "CommitMode" which can be used
	 * by e-paper displays to choose the driving waveforms for this operation.
	 */
	virtual void commit(int x, int y, int w, int h, CommitMode mode) = 0;

	/**
	 * Returns the width of the display in pixels.
	 */
	virtual unsigned int width() const = 0;

	/**
	 * Returns the height of the display in pixels.
	 */
	virtual unsigned int height() const = 0;

	/**
	 * Clips the given x-coordinate to a valid x-coordinate within the screen
	 * region.
	 */
	int clipx(int x)
	{
		return (x < 0) ? 0 : (((unsigned int)x >= width()) ? width() : x);
	}

	/**
	 * Clips the given y-coordinate to a valid y-coordinate within the screen
	 * region.
	 */
	int clipy(int y)
	{
		return (y < 0) ? 0 : (((unsigned int)y >= height()) ? height() : y);
	}
};

/**
 * Specialisation of the Display class that implements the blit() and fill()
 * functions assuming that the back-buffer is a memory region.
 */
class MemDisplay : public Display {
protected:
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
		uint32_t conv(const RGB &c) const
		{
			return ((uint32_t(c.r) >> rr) << rl) |
			       ((uint32_t(c.g) >> gr) << gl) |
			       ((uint32_t(c.b) >> br) << bl);
		}

		/**
		 * Computes the bytes per pixel.
		 */
		uint8_t bypp() const { return (bpp + 7U) >> 3U; }
	};

	/**
	 * Must be implemented by the display and return the pointer at the
	 * back-buffer.
	 */
	virtual uint8_t *buf() const = 0;

	/**
	 * Returns the width of one line in bytes.
	 */
	virtual unsigned int stride() const = 0;

	/**
	 * Returns a reference at the color layout used in the memory display.
	 */
	virtual const ColorLayout &layout() const = 0;

public:
	void blit(const uint8_t *mask, int stride, const RGB &fg, const RGB &bg,
	                  int x, int y, int w, int h) override;

	void fill(const RGB &c, int x, int y, int w, int h) override;
};

}  // namespace inktty

#endif /* INKTTY_GFX_DISPLAY_HPP */
