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
#include <memory>

#include <inktty/utils/color.hpp>
#include <inktty/utils/geometry.hpp>

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
		Partial,

		/**
		 * Perform a partial update with a forced flash-to-black.
		 */
		Flash_Partial,

		/**
		 * Perform an with low quality as used for UI elements.
		 */
		Flash_Ui,

		/**
		 * Perform a full, high-quality update.
		 */
		Full,

		/**
		 * Perform a fast update with low-quality.
		 */
		Fast,
	};

	/**
	 * The display consists of three independent layers that are composed into a
	 * single image when commiting to the display.
	 */
	enum class Layer {
		/**
		 * Layer containing the background image. The handling of transparency
		 * in this layer is implementation-defined.
		 */
		Background,

		/**
		 * The presentation layer is supposed to contain the actual characters
		 * or UI elements that are being displayed.
		 */
		Presentation,

		/**
		 * The foreground layer contains cursors or other UI elements that
		 * should be drawn independently on top of the other two layers.
		 */
		Foreground
	};

	/**
	 * Draw mode to use when blitting onto a surface.
	 */
	enum class DrawMode {
		/**
		 * Writes the mask to the target layer. Blends with already existing
		 * content.
		 */
		Write,

		/**
		 * Removes the mask from the target layer, undoing the "write"
		 * operation. This (to some degree) allows to remove overlapping shapes
		 * without having to re-render large parts of the screen.
		 */
		Remove
	};

	/**
	 * Virtual destructor. Does nothing in this interface definition.
	 */
	virtual ~Display();

	/**
	 * Locks the display. Drawing and commit operations are now allowed.
	 * Performing a draw or commit operation without locking the surface has no
	 * effect. Note that locking must be reference counted, i.e. multiple calls
	 * to lock()/unlock() must be possible as long as the number of calls to
	 * lock()/unlock() is equal. The size of the display must not change while
	 * the display is locked.
	 *
	 * @return The bounding rectangle of the display.
	 */
	virtual Rect lock() = 0;

	/**
	 * Unlocks the surface. Whenever unlock() has been called as often as
	 * lock(), all queued changes are commited to the display.
	 */
	virtual void unlock() = 0;

	/**
	 * Commits the content written to the display to the screen in the given
	 * region. This function allows to specify a "CommitMode" which can be used
	 * by e-paper displays to choose the driving waveforms for this operation.
	 * Note that the actual commit operation takes place once unlock() is called
	 * and after all drawing operations occured.
	 */
	virtual void commit(const Rect &r = Rect(),
	                    CommitMode mode = CommitMode::Full) = 0;

	/**
	 * Blits the given mask bitmap (i.e. an 8-bit grayscale image) to the
	 * display at location (x, y).
	 *
	 * @param mask is a pointer at a 8-bit alpha-mask. Determines how
	 * @param stride is the width of one line in the mask image in bytes.
	 * @param fg is the foreground color that should be used.
	 * @param bg is the background color that should be used.
	 * @param r is the target rectangle. The width and height of this rectangle
	 * also determines the width/height of the source image.
	 */
	virtual void blit(Layer layer, const RGBA &c, const uint8_t *mask,
	                  size_t stride, const Rect r,
	                  DrawMode mode = DrawMode::Write) = 0;

	/**
	 * Fills the specified rectangle with the given solid colour.
	 */
	virtual void fill(Layer layer, const RGBA &c = RGBA::White,
	                  const Rect &r = Rect()) = 0;
};

/**
 * The MemoryDisplay class implements most of the Display interface and all
 * drawing operations. The actual display backend must implement the protected
 * do_lock(), do_unlock() functions.
 */
class MemoryDisplay : public Display {
protected:
	/**
	 * Structure for storing the accumulated commit requests.
	 */
	struct CommitRequest {
		Rect r;
		CommitMode mode;
	};

	/**
	 * Must return the current size of the display and not change it until
	 * unlock() is called.
	 */
	virtual Rect do_lock() = 0;

	/**
	 * Unlocks the display.
	 */
	virtual void do_unlock(const CommitRequest *begin, const CommitRequest *end,
	                       const RGBA *buf, size_t stride) = 0;

private:
	class Impl;
	std::unique_ptr<Impl> m_impl;

public:
	/**
	 * Default constructor. Creates the internal implementation.
	 */
	MemoryDisplay();

	/**
	 * Destructor, destroys the internal implementation.
	 */
	~MemoryDisplay();

	/**
	 * Locks the display. Drawing and commit operations are now allowed.
	 * Performing a draw or commit operation without locking the surface has no
	 * effect.
	 *
	 * @return The bounding rectangle of the display.
	 */
	Rect lock() override;

	/**
	 * Unlocks the surface. Actually commits all queued changes to the display.
	 */
	void unlock() override;

	/**
	 * Commits the content written to the display to the screen in the given
	 * region. This function allows to specify a "CommitMode" which can be used
	 * by e-paper displays to choose the driving waveforms for this operation.
	 * Note that the actual commit operation takes place once unlock() is called
	 * and after all drawing operations occured.
	 */
	void commit(const Rect &r = Rect(),
	            CommitMode mode = CommitMode::Full) override;

	/**
	 * Blits the given mask bitmap (i.e. an 8-bit grayscale image) to the
	 * display at location (x, y).
	 *
	 * @param mask is a pointer at a 8-bit alpha-mask. Determines how
	 * @param stride is the width of one line in the mask image in bytes.
	 * @param fg is the foreground color that should be used.
	 * @param bg is the background color that should be used.
	 * @param r is the target rectangle. The width and height of this rectangle
	 * also determines the width/height of the source image.
	 */
	void blit(Layer layer, const RGBA &c, const uint8_t *mask, size_t stride,
	          const Rect r,

	          DrawMode mode = DrawMode::Write) override;

	/**
	 * Fills the specified rectangle with the given solid colour.
	 *
	 * @param c is the colour the rectangle should be filled with.
	 * @param x is the x-coordinate of the top-left corner of the rectangle that
	 * should be filled.
	 * @param y is the y-coordinate of the top-left corner of the rectangle that
	 * should be filled.
	 */
	void fill(Layer layer, const RGBA &c = RGBA::White,
	          const Rect &r = Rect()) override;
};
}  // namespace inktty

#endif /* INKTTY_GFX_DISPLAY_HPP */
