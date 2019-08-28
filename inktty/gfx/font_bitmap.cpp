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

#include <unordered_map>

#include <inktty/gfx/font_bitmap.hpp>
#include <inktty/gfx/font_cache.hpp>

#include <inktty/fontdata/font_8x16.h>

namespace inktty {

/******************************************************************************
 * Class FontBitmap::Impl                                                     *
 ******************************************************************************/

class FontBitmap::Impl {
private:
	FontCache m_cache;
	std::unordered_map<uint32_t, int> m_codepage;
	const uint8_t *m_mem;
	int m_stride, m_w, m_h, m_n;

public:
	Impl(const uint8_t *mem, int stride, int width, int height, int n,
	     const uint32_t *codepage)
	    : m_cache(1024),
	      m_mem(mem),
	      m_stride(stride),
	      m_w(width),
	      m_h(height),
	      m_n(n) {
		// Create a map mapping from Unicode glyphs to font indices
		for (int i = 0; i < m_n; i++) {
			m_codepage.emplace(codepage[i], i);
		}
	}

	const GlyphBitmap *render(uint32_t glyph, unsigned int, bool,
	                          unsigned int orientation) {
		// Check whether the glyph is cached, if yes, just return the cached
		// glyph
		GlyphMetadata metadata{glyph, 0, false, orientation};
		GlyphBitmap *res = m_cache.get(metadata);
		if (res) {
			return res;
		}

		// Try to convert the given glyph into the corresponding font index
		auto it = m_codepage.find(glyph);
		if (it == m_codepage.end()) {
			return nullptr;
		}

		// Create a new glyph entry
		const size_t idx = it->second;
		const size_t w = (orientation & 1) ? m_h : m_w;
		const size_t h = (orientation & 1) ? m_w : m_h;
		const size_t stride = (((unsigned int)w + 15U) / 16U) * 16U;
		res = m_cache.put(0, 0, w, h, stride, metadata);

		// Render the glyph
		const uint8_t *src = m_mem + idx * m_stride * m_h;
		uint8_t *tar = res->buf();
		for (int sy = 0; sy < m_h; sy += 1) {
			for (int sx_ = 0; sx_ < m_w; sx_ += 8) {
				for (int sxo = 0; sxo < 8; sxo += 1) {
					// Compute the actual source x
					const int sx = sx_ + 7 - sxo;

					// Compute the target cordinates
					int tx = -1, ty = -1;
					switch (orientation) {
						case 0:
							tx = sx;
							ty = sy;
							break;
						case 1:
							tx = sy;
							ty = (m_w - 1) - sx;
							break;
						case 2:
							tx = (m_w - 1) - sx;
							ty = (m_h - 1) - sy;
							break;
						case 3:
							tx = (m_h - 1) - sy;
							ty = sx;
							break;
					}
					if (tx < 0 || tx >= (int)w || ty < 0 || ty >= (int)h) {
						continue;
					}

					// Set the pixel value
					tar[ty * stride + tx] = ((*src) & (1 << sxo)) ? 255 : 0;
				}
				src++;
			}
		}
		return res;
	}

	MonospaceFontMetrics metrics(int) const {
		return MonospaceFontMetrics{m_w, m_h, 0};
	}
};

/******************************************************************************
 * Class FontBitmap                                                           *
 ******************************************************************************/

FontBitmap::FontBitmap(const uint8_t *mem, int stride, int width, int height, int n,
                       const uint32_t *codepage)
    : m_impl(new FontBitmap::Impl(mem, stride, width, height, n, codepage)) {}

FontBitmap::~FontBitmap() {
	// Do nothing here
}

const GlyphBitmap *FontBitmap::render(uint32_t glyph, unsigned int size,
                                      bool monochrome,
                                      unsigned int orientation) {
	return m_impl->render(glyph, size, monochrome, orientation);
}

MonospaceFontMetrics FontBitmap::metrics(int size) const {
	return m_impl->metrics(size);
}

/******************************************************************************
 * Static Initialisations                                                     *
 ******************************************************************************/

FontBitmap FontBitmap::Font8x16(fontdata_8x16, 1, 8, 16, 255, fontdata_8x16_codepage);

}  // namespace inktty

