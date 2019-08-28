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
 * @file font_bitmap.hpp
 *
 * Provides a 8x16 pixel fallback font
 *
 * @author Andreas Stöckel
 */

#ifndef INKTTY_GFX_FONT_BITMAP_HPP
#define INKTTY_GFX_FONT_BITMAP_HPP

#include <memory>

#include <inktty/gfx/font.hpp>

namespace inktty {
/**
 * Fixed bitmap font used as a fallback if not TTF font is specified.
 */
class FontBitmap : public Font {
private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

public:
	static FontBitmap Font8x16;

	FontBitmap(const uint8_t *mem, int stride, int width, int height, int n,
	           const uint32_t *codepage);

	~FontBitmap() override;

	const GlyphBitmap *render(uint32_t glyph, unsigned int size,
	                          bool monochrome = false,
	                          unsigned int orientation = 0) override;

	MonospaceFontMetrics metrics(int size) const override;
};

}  // namespace inktty

#endif /* INKTTY_GFX_FONT_BITMAP_HPP */
