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

#include <algorithm>
#include <list>
#include <stdexcept>
#include <unordered_map>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H
#include FT_MODULE_H

#include <inktty/gfx/font.hpp>

namespace inktty {
namespace {

/******************************************************************************
 * Class FreetypeLibrary                                                      *
 ******************************************************************************/

/**
 * Helper class used to initialize free type. Do not create instances of this
 * class directly but used the singleton instance FreetypeLibrary::inst.
 */
class Freetype {
private:
	FT_Library m_library;

	const char *ft_error_string(FT_Error code) {
#include FT_FREETYPE_H
#undef __FTERRORS_H__
#define FT_ERRORDEF(e, v, s) \
	case e:                  \
		return s;
#define FT_ERROR_START_LIST switch (code) {
#define FT_ERROR_END_LIST }
#include FT_ERRORS_H
		return "Unkown freetype error";
	}

	FT_Error handle_err(FT_Error code) {
		if (code != FT_Err_Ok) {
			throw std::runtime_error(ft_error_string(code));
		}
		return code;
	}

	/**
	 * Creates the FreetypeLibrary instance.
	 */
	Freetype() {
		FT_Bool value;
		handle_err(FT_Init_FreeType(&m_library));

		handle_err(FT_Property_Set(m_library, "autofitter",
			                           "no-stem-darkening",
			                           &(value = false)));
		handle_err(FT_Property_Set(m_library, "autofitter",
			                           "warping",
			                           &(value = true)));
	}

public:
	/**
	 * Singleton instance.
	 */
	static Freetype library;

	operator FT_Library() { return m_library; }
};

Freetype Freetype::library;

}  // namespace

/******************************************************************************
 * Class Font::Impl                                                           *
 ******************************************************************************/

static uint32_t PROBE_CHARS[] = {
    '[',    ']', '(', ')',
    'A',     // Latin capital letter A
    'O',     // Latin capital letter O
    'j',     // Latin small letter j
    'l',     // Latin small letter l
    'w',     // Latin small letter w
    'y',     // Latin small letter y
    0x0391,  // Greek capital letter Α
    0x0398,  // Greek capital letter Θ
    0x03B1,  // Greek small letter α
    0x03B2,  // Greek small letter β
    0x03B6,  // Greek small letter ζ
    0x03C1,  // Greek small letter ρ
    0x0402,  // Cyrillic capital letter Ђ
    0x0410,  // Cyrillic capital letter А
    0x0424,  // Cyrillic capital letter Ф
    0x0428,  // Cyrillic capital letter Ш
    0x0416,  // Cyrillic capital letter Ж
    0x044B,  // Cyrillic small letter Ы
    0x0430,  // Cyrillic small letter а
    0x0443,  // Cyrillic small letter y
    0x0457,  // Cyrillic small letter ј
    0x05D0,  // Hebrew letter א
    0x05DC,  // Hebrew letter ל
    0x05E9,  // Hebrew letter ש
    0x05E5,  // Hebrew letter ץ
    0x2588,  // Full block █
};

class Font::Impl {
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
	 * Freetype font face.
	 */
	FT_Face m_face;

	/**
	 * Temporary freetype bitmap.
	 */
	FT_Bitmap m_tmp_bmp;

	/**
	 * Datastructure holding information about the monospace grid size.
	 */
	MonospaceFontMetrics m_metrics;

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
	 * Dots per inch on the screen. Used to convert the given sizes in points
	 * to pixels.
	 */
	unsigned int m_dpi;

	/**
	 * Maximum number of entires in the glyph cache.
	 */
	unsigned int m_max_cache_size;

	/**
	 * Searches for the glyph with the given properties in the glyph cache. If
	 * found, moves the glyph to the beginning of the glyph list so it is not
	 * freed.
	 */
	GlyphBitmap *from_cache(const GlyphMetadata &metadata) {
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

	/**
	 * Creates a new Glyph instance and adds it to the cache. Removes the least
	 * recently used glyph from the cache if the cache is over-full.
	 */
	GlyphBitmap *add_to_cache(int x, int y, unsigned int w, unsigned int h,
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

	/**
	 * Computes font metrics from the loaded font face. This function iterates
	 * over a list of probe characters from different scripts and determines the
	 * corresponding cell size.
	 */
	static MonospaceFontMetrics compute_monospace_font_metrics(
	    FT_Face face, unsigned int dpi) {
		// Fix the character size to simplify calculations later on
		FT_Set_Char_Size(face, 0, 512 * 64, dpi, dpi);

		// Iterate over all probe chars and calculate the metrics
		int32_t x0 = 0x7FFFFFFF, y0 = 0x7FFFFFFF;
		int32_t x1 = -0x80000000, y1 = -0x80000000;
		int32_t origin_y = -0x80000000;
		const size_t n = sizeof(PROBE_CHARS) / sizeof(PROBE_CHARS[0]);
		for (size_t i = 0; i < n; i++) {
			/* Lookup the glyph index */
			const FT_ULong glyph = PROBE_CHARS[i];
			const FT_UInt glyph_idx = FT_Get_Char_Index(face, glyph);
			if (glyph_idx == 0) {
				continue;
			}

			/* Compute the glyph metrics */
			FT_Error err =
			    FT_Load_Glyph(face, glyph_idx, FT_LOAD_BITMAP_METRICS_ONLY);
			if (err != FT_Err_Ok) {
				continue; /* Return empty glyph */
			}

			/* Compute the glyph bounding box */
			const FT_Glyph_Metrics *metrics = &face->glyph->metrics;
			const int32_t gx0 = metrics->horiBearingX;
			const int32_t gx1 = metrics->horiAdvance;
			const int32_t gy0 = -metrics->horiBearingY;
			const int32_t gy1 = -metrics->horiBearingY + metrics->height;
			const int32_t gorigin_y = metrics->horiBearingY;

			/* Update the global bounding box size */
			x0 = std::min(gx0, x0), x1 = std::max(gx1, x1);
			y0 = std::min(gy0, y0), y1 = std::max(gy1, y1);
			origin_y = std::max(gorigin_y, origin_y);
		}

		return MonospaceFontMetrics{(x1 - x0), (y1 - y0), origin_y};
	}

	static void copy_rotated(const uint8_t *src, size_t src_stride,
	                         size_t src_w, size_t src_h, uint8_t *tar,
	                         size_t tar_stride, unsigned int orientation) {
		switch (orientation % 4) {
			case 0U:
				for (size_t j = 0U; j < src_h; j++) {
					const uint8_t *src_row = src + src_stride * j;
					uint8_t *tar_row = tar + tar_stride * j;
					std::copy(src_row, src_row + src_w, tar_row);
				}
				break;
			case 1U:
				for (size_t j = 0U; j < src_h; j++) {
					const uint8_t *src_row = src + src_stride * j;
					for (size_t i = 0U; i < src_w; i++) {
						(tar + tar_stride * (src_w - 1 - i))[j] = src_row[i];
					}
				}
				break;
			case 2U:
				for (size_t j = 0U; j < src_h; j++) {
					const uint8_t *src_row = src + src_stride * j;
					uint8_t *tar_row = tar + tar_stride * (src_h - 1 - j);
					std::copy(src_row, src_row + src_w, tar_row);
				}
				break;
			case 3U:
				for (size_t j = 0U; j < src_h; j++) {
					const uint8_t *src_row = src + src_stride * j;
					for (size_t i = 0U; i < src_w; i++) {
						(tar + tar_stride * i)[src_h - 1 - j] = src_row[i];
					}
				}
				break;
		}
	}

public:
	Impl(const char *ttf_file, unsigned int dpi, unsigned int max_cache_size)
	    : m_dpi(dpi), m_max_cache_size(max_cache_size) {
		// Try to load the font face
		FT_Error err = FT_New_Face(Freetype::library, ttf_file, 0, &m_face);
		if (err == FT_Err_Unknown_File_Format) {
			throw std::runtime_error(
			    "Specified font file format not recognized.");
		} else if (err != FT_Err_Ok) {
			throw std::runtime_error("Error while loading font.");
		}

		// Make sure the font is scaleable
		if (!FT_IS_SCALABLE(m_face)) {
			throw std::runtime_error("Font is not scaleable!");
		}

		// Compute the monospace font metrics
		m_metrics = compute_monospace_font_metrics(m_face, m_dpi);

		// Allocate a temporary bitmap
		FT_Bitmap_Init(&m_tmp_bmp);
	}

	~Impl() {
		FT_Bitmap_Done(Freetype::library, &m_tmp_bmp);
		FT_Done_Face(m_face);
	}

	void clear() {
		m_glyph_cache_map.clear();
		m_glyph_cache.clear();
	}

	const GlyphBitmap *render(uint32_t glyph, unsigned int size,
	                          bool monochrome, unsigned int orientation) {
		// Check whether the glyph is cached, if yes, just return the cached
		// glyph
		GlyphMetadata metadata{glyph, size, monochrome, orientation};
		GlyphBitmap *res = from_cache(metadata);
		if (res) {
			return res;
		}

		// Set the font size and transformation
		FT_Error err = FT_Set_Char_Size(m_face, 0, size, m_dpi, m_dpi);
		if (err != FT_Err_Ok) {
			throw std::runtime_error("Error while setting font size.");
		}

		// Lookup the glyph index
		const FT_UInt glyph_idx = FT_Get_Char_Index(m_face, glyph);
		if (glyph_idx == 0) {
			return nullptr;  // Glyph is not in this font
		}

		// Load the glyph and render it
		int flags = FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT;
		if (monochrome) {
			flags |= FT_LOAD_TARGET_MONO;
		}
		err = FT_Load_Glyph(m_face, glyph_idx, flags);
		if (err != FT_Err_Ok) {
			return nullptr;  // Return empty glyph
		}

		// Allocate the glyph on the cache
		const FT_GlyphSlot slot = m_face->glyph;
		const FT_Bitmap *bmp = &slot->bitmap;

		// Convert the bitmap to the right format
		if (bmp->pixel_mode != FT_PIXEL_MODE_GRAY) {
			FT_Bitmap_Convert(Freetype::library, bmp, &m_tmp_bmp, 1);
			bmp = &m_tmp_bmp;
		}
		if (bmp->num_grays == 2) {
			for (unsigned int i = 0; i < bmp->rows; i++) {
				unsigned char *p = bmp->buffer + i * bmp->pitch;
				for (unsigned int j = 0; j < bmp->width; j++) {
					*p = *p ? 255 : 0;
					p++;
				}
			}
		}

		// Determine the x-/y-offset depending on the orientation
		const MonospaceFontMetrics m = metrics(size);
		int x = 0, y = 0;
		switch (orientation % 4) {
			case 0:
				x = slot->bitmap_left;
				y = m.origin_y - slot->bitmap_top;
				break;
			case 1:
				x = m.origin_y - slot->bitmap_top;
				y = slot->bitmap_left;
				break;
			case 2:
				x = slot->bitmap_left;
				y = slot->bitmap_top - m.origin_y + (m.cell_height - bmp->rows);
				break;
			case 3:
				x = slot->bitmap_top - m.origin_y + (m.cell_height - bmp->rows);
				y = slot->bitmap_left;
				break;
		}

		// Create the glyph
		const size_t w = (orientation & 1) ? bmp->rows : bmp->width;
		const size_t h = (orientation & 1) ? bmp->width : bmp->rows;
		const size_t stride = ((w + 15U) / 16U) * 16U;
		res = add_to_cache(x, y, w, h, stride, metadata);

		// Copy the glyph to the output glyph
		copy_rotated(bmp->buffer, bmp->pitch, bmp->width, bmp->rows, res->buf(),
		             stride, orientation);
		return res;
	}

	MonospaceFontMetrics metrics(int size) const {
		const int num = size, den = 512 * 64 * 64;
		return MonospaceFontMetrics{m_metrics.cell_width * num / den,
		                            m_metrics.cell_height * num / den,
		                            m_metrics.origin_y * num / den};
	}
};

/******************************************************************************
 * Class Font                                                                 *
 ******************************************************************************/

Font::Font(const char *ttf_file, unsigned int dpi, unsigned int max_cache_size)
    : m_impl(std::unique_ptr<Impl>(new Impl(ttf_file, dpi, max_cache_size))) {}

Font::~Font() {
	// Do nothing here
}

void Font::clear() { m_impl->clear(); }

const GlyphBitmap *Font::render(uint32_t glyph, unsigned int size,
                                bool monochrome, unsigned int orientation) {
	return m_impl->render(glyph, size, monochrome, orientation);
}

MonospaceFontMetrics Font::metrics(int size) const {
	return m_impl->metrics(size);
}

}  // namespace inktty
