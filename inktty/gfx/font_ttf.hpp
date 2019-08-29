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

#ifndef INKTTY_GFX_FONT_TTF_HPP
#define INKTTY_GFX_FONT_TTF_HPP

#include <config.h>
#ifdef HAS_FREETYPE

#include <memory>

#include <inktty/gfx/font.hpp>

namespace inktty {
/**
 * The Font class represents a single font as it is stored in a TrueType font
 * file. It allows to render individual glyphs from the font to a bitmap,
 * assuming that the font is a monospace font.
 */
class FontTTF : public Font {
private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

public:
	FontTTF(const char *ttf_file, unsigned int dpi = 96,
	        unsigned int max_cache_size = 1000);

	/**
	 * Destroys the open font handle.
	 */
	~FontTTF() override;

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
	                          unsigned int orientation = 0) override;

	/**
	 * Returns the font metrics assuming this font is a monospace font.
	 *
	 * @param size is the size of the font in 1/64th of a point.
	 * @return a datastructure containing information about the cell width and
	 * height in pixels.
	 */
	MonospaceFontMetrics metrics(int size) const override;
};

}  // namespace inktty

#endif /* HAS_FREETYPE */
#endif /* INKTTY_GFX_FONT_TTF_HPP */
