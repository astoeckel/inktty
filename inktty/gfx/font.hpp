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

/**
 * @file font.hpp
 *
 * This file provides a font render and glyph cache for monospace fonts. The
 * font renderer is based on FreeType2 and supports both anti-aliased and
 * auto-hinted monochrome rendering of glyphs. Having access to a high-quality
 * monochrome renderings is important since e-paper displays are faster when
 * writing 1-bit graphics.
 *
 * @author Andreas Stöckel
 */

#ifndef INKTTY_GFX_FONT_HPP
#define INKTTY_GFX_FONT_HPP

#include <cstdint>
#include <memory>
#include <vector>

namespace inktty {
/* Forward declaration */
class Font;

/**
 * The GlyphMetadata structure stores information about the Glyph.
 */
struct GlyphMetadata {
	/**
	 * Unicode character.
	 */
	uint32_t glyph;

	/**
	 * Size in 1/64th points.
	 */
	unsigned int size;

	/**
	 * If true, the glyph was rendered monochrome, that is without
	 * anti-aliasing.
	 */
	bool monochrome;

	/**
	 * Rotation of the glyph in 90° steps.
	 */
	unsigned int orientation;

	/**
	 * Returns true if two GlyphMetadata instances are equal.
	 */
	bool operator==(const GlyphMetadata &o) const
	{
		return glyph == o.glyph && size == o.size &&
		       monochrome == o.monochrome && orientation == o.orientation;
	}
};

/**
 * Descriptor describing a single rendered glyph.
 */
class GlyphBitmap {
private:
	/**
	 * Vector storing the glyph bitmap.
	 */
	std::vector<uint8_t> m_buf;

public:
	/**
	 * Offset in x-direction from the top-left corner of the corresponding
	 * monospace cell.
	 */
	int x;

	/**
	 * Offset in y-direction from the top-left corner of the corresponding
	 * monospace cell.
	 */
	int y;

	/**
	 * Width of the glyph in pixels.
	 */
	unsigned int w;

	/**
	 * Height of the glyph in pixels.
	 */
	unsigned int h;

	/**
	 * Size of one line in bytes.
	 */
	unsigned int stride;

	/**
	 * Glyph metadata.
	 */
	GlyphMetadata metadata;

	/**
	 * Default constructor.
	 */
	GlyphBitmap() : x(0), y(0), w(0), h(0), stride(0) {}

	/**
	 * Allocates memory for a glyph bitmap of the given size.
	 *
	 * @param x is the x-offset that should be applied when blitting the bitmap.
	 * @param y is the y-offset that should be applied when blitting the bitmap.
	 * @param w is the width of the bitmap in pixels.
	 * @param h is the height of the bitmap in pixels.
	 * @param stride is the size of one bitmap line in bytes.
	 * @param metadata is the metadata describing the glyph and its style.
	 */
	GlyphBitmap(int x, int y, unsigned int w, unsigned int h,
	            unsigned int stride, GlyphMetadata metadata)
	    : m_buf(h * stride, 0U),
	      x(x),
	      y(y),
	      w(w),
	      h(h),
	      stride(stride),
	      metadata(metadata)
	{
	}

	/**
	 * Returns a pointer at the bitmap containing the Glyph data.
	 */
	const uint8_t *buf() const { return &m_buf[0]; }

	/**
	 * Returns a pointer at the bitmap containing the Glyph data.
	 */
	uint8_t *buf() { return &m_buf[0]; }
};

/**
 * Metrics extracted from the font under the assumption that the font is going
 * to be rendered as a monospace font.
 */
struct MonospaceFontMetrics {
	/**
	 * Size of a monospace cell in pixels.
	 */
	int cell_width, cell_height;

	/**
	 * Position of the font origin point on the y-axis in pixels.
	 */
	int origin_y;
};

/**
 * The Font class represents a single font as it is stored in a TrueType font
 * file. It allows to render individual glyphs from the font to a bitmap,
 * assuming that the font is a monospace font.
 */
class Font {
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;

public:
	/**
	 * Creates a new instance of the Font class bound to the given True Type
	 * Font file.
	 *
	 * @param ttf_file is the name of the True Type Font file that should be
	 * opened.
	 * @param dpi is the resolution of the display. This number is used when
	 * converting between sizes in points and pixels.
	 * @param max_cache_size is the maximum number of glyphs that is stored
	 * in the cache.
	 */
	Font(const char *ttf_file, unsigned int dpi = 96,
	     unsigned int max_cache_size = 1000);

	/**
	 * Destroys the open font handle.
	 */
	~Font();

	/**
	 * Clears the font cache.
	 */
	void clear();

	/**
	 * Renders the given Unicode glyph to a bitmap with the specified style to a
	 * Glyph bitmap of the given size. Returns nullptr if there was an error
	 * while rendering the character or the glyph does not exist.
	 *
	 * @param glyph is the Unicode codepoint.
	 * @param size is the size of the glyph in 1/64 points.
	 * @param monochrome if true the glyph is rendered without antialiasing.
	 * @param orientation is the orientation of the glyph in multiples of 90
	 * degrees counter clock-wise.
	 * @return a pointer at the Glyph or nullptr if the glyph could not be
	 * extracted from the font.
	 */
	const GlyphBitmap *render(uint32_t glyph, unsigned int size,
	                          bool monochrome = false,
	                          unsigned int orientation = 0);

	/**
	 * Returns the font metrics assuming this font is a monospace font.
	 *
	 * @param size is the size of the font in 1/64th of a point.
	 * @return a datastructure containing information about the cell width and
	 * height in pixels.
	 */
	MonospaceFontMetrics metrics(int size) const;
};

}  // namespace inktty

#endif /* INKTTY_GFX_FONT_HPP */
