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

#ifndef INKTTY_GFX_FONT_CACHE_HPP
#define INKTTY_GFX_FONT_CACHE_HPP

#include <list>
#include <unordered_map>

#include <inktty/gfx/font.hpp>

namespace inktty {

class FontCache {
private:
	/**
	 * Used to compute the hash of the GlyphMetadata class.
	 */
	struct GlyphMetadataHasher {
		size_t operator()(const GlyphMetadata &m) const {
			return (m.glyph * 48923) ^ (m.size * 28147) ^ (m.monochrome << 5) ^
			       (m.orientation * 392);
		}
	};

	/**
	 * Map storing references at the already rendered glyphs.
	 */
	std::unordered_map<GlyphMetadata, std::list<GlyphBitmap>::iterator,
	                   GlyphMetadataHasher>
	    m_glyph_cache_map;

	/**
	 * List containing the rendered glyphs. Last accessed elements are at the
	 * end of the list. The first element in the list is deleted whenever there
	 * are too many elements in the cache.
	 */
	std::list<GlyphBitmap> m_glyph_cache;

	/**
	 * Maximum number of entires in the glyph cache.
	 */
	unsigned int m_max_cache_size;

public:
	/**
	 * Creates a new FontCache instance with the given maximum number of glyphs.
	 */
	FontCache(unsigned int max_cache_size);

	/**
	 * Clears the font cache.
	 */
	void clear() {
		m_glyph_cache.clear();
		m_glyph_cache_map.clear();
	}

	/**
	 * Searches for the glyph with the given properties in the glyph cache. If
	 * found, moves the glyph to the beginning of the glyph list so it is not
	 * freed.
	 */
	GlyphBitmap *get(const GlyphMetadata &metadata);

	/**
	 * Creates a new Glyph instance and adds it to the cache. Removes the least
	 * recently used glyph from the cache if the cache is over-full.
	 */
	GlyphBitmap *put(int x, int y, unsigned int w, unsigned int h,
	                 unsigned int stride, const GlyphMetadata &metadata);
};

}  // namespace inktty

#endif /* INKTTY_GFX_FONT_CACHE_HPP */
