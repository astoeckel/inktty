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
 * @file color.hpp
 *
 * Contains routines and datastructures for colour representation and
 * conversion.
 *
 * @author Andreas Stöckel
 */

#ifndef INKTTY_UTILS_COLOR_HPP
#define INKTTY_UTILS_COLOR_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace inktty {
/**
 * A struct describing a 32-bit RGBA colour.
 */
struct RGBA {
	/**
	 * R, G, B value.
	 */
	uint8_t b, g, r, a;

	/**
	 * Default constructor, initialises all values to zero.
	 */
	constexpr RGBA() : b(0U), g(0U), r(0U), a(0U) {}

	/**
	 * Constructs a new colour from the given RGBA hex colour code. Sets the
	 * alpha channel to maximum opacity.
	 */
	constexpr RGBA(uint32_t hex)
	    : b(hex & 0xFFU),
	      g((hex & 0xFF00U) >> 8U),
	      r((hex & 0xFF0000U) >> 16U),
	      a(0xFFU) {}

	/**
	 * Constructs a new RGBA colour.
	 */
	constexpr RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFFU)
	    : b(b), g(g), r(r), a(a) {}

	static const RGBA Black;
	static const RGBA White;
	static const RGBA SolarizedBlack;
	static const RGBA SolarizedWhite;

	bool operator==(const RGBA &o) const {
		return (r == o.r) && (b == o.b) && (g == o.g) && (a == o.a);
	}

	bool operator!=(const RGBA &o) const {
		return !((*this) == o);
	}	

	RGBA premultiply_alpha() const {
		return RGBA(
			uint16_t(r) * a / 255,
			uint16_t(g) * a / 255,
			uint16_t(b) * a / 255,
			a);
	}
};

/**
 * A palette which defines up to 256 colours.
 */
class Palette {
private:
	/**
	 * Array storing the actual palette entries.
	 */
	std::array<RGBA, 256> m_entries;

	/**
	 * Contains the size of this table.
	 */
	size_t m_size;

public:
	/**
	 * Constructs a new empty palette of the given size.
	 */
	Palette(int size) : m_size(size) {
		/* Make sure the size is valid */
		assert(size >= 0 && size <= 256);

		/* Initialise the memory */
		for (int i = 0; i < size; i++) {
			m_entries[i] = RGBA();
		}
	}

	/**
	 * Constructs a new palette by copying the given structure to the internal
	 * memory.
	 */
	Palette(int size, const RGBA *data) : Palette(size) {
		for (int i = 0; i < size; i++) {
			m_entries[i] = data[i];
		}
	}

	static const Palette Default16Colours;
	static const Palette Solarized16Colours;
	static const Palette Default256Colours;

	/**
	 * Returns the size of the palette.
	 */
	size_t size() const { return m_size; }

	RGBA &operator[](int i) {
		if (i >= 0 && size_t(i) < m_size) {
			return m_entries[i];
		}
		return const_cast<RGBA &>(RGBA::Black);
	}

	const RGBA &operator[](int i) const {
		return (*const_cast<Palette *>(this))[i];
	}
};
}  // namespace inktty

#endif /* INKTTY_UTILS_COLOR_HPP */
