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

#include <inktty/gfx/font_cache.hpp>

namespace inktty {

FontCache::FontCache(unsigned int max_cache_size)
    : m_max_cache_size(max_cache_size) {}

GlyphBitmap *FontCache::get(const GlyphMetadata &metadata) {
		// Construct the cache key and look it up in the glyph cache
		auto it = m_glyph_cache_map.find(metadata);
		if (it == m_glyph_cache_map.end()) {
			return nullptr;
		}

		// Glyph was found, move to the beginning of the glyph list
		auto bmp_it = it->second;
		if (bmp_it != m_glyph_cache.begin()) {
			m_glyph_cache.splice(m_glyph_cache.begin(), m_glyph_cache, bmp_it,
			                     std::next(bmp_it));
		}
		return &(*bmp_it);
}

GlyphBitmap *FontCache::put(int x, int y, unsigned int w, unsigned int h,
                            unsigned int stride,
                            const GlyphMetadata &metadata) {
		// Add a new glyph to the cache
		m_glyph_cache.emplace_front(x, y, w, h, stride, metadata);
		auto bmp_it = m_glyph_cache.begin();

		// Add the cache key to the map
		m_glyph_cache_map.emplace(metadata, bmp_it);

		// Delete the oldest entry in the glyph list if there are too many
		// glyphs in the cache
		if (m_glyph_cache.size() > m_max_cache_size) {
			/* Fetch the old glyph instance */
			auto old_bmp_it = std::prev(m_glyph_cache.end());

			/* Remove it from the glyph cache map */
			m_glyph_cache_map.erase(old_bmp_it->metadata);

			/* Remove it from the glyph cache */
			m_glyph_cache.erase(old_bmp_it);
		}

		return &(*bmp_it);
}

}  // namespace inktty

