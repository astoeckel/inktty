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

#ifndef INKTTY_BACKENDS_EPAPER_EMULATION_HPP
#define INKTTY_BACKENDS_EPAPER_EMULATION_HPP

#include <inktty/gfx/display.hpp>

namespace inktty {
namespace epaper_emulation {
/**
 * EPaperEmulation::update() is used in the SDLBackend epaper emulation
 * mode. This mode is mostly used for development purposes.
 */
void update(uint8_t *tar, size_t tar_stride, const ColorLayout &tar_layout,
            const RGBA *src, size_t src_stride, int x0, int y0, int x1, int y1,
            UpdateMode mode);

/**
 * Converts the given colour to a 4-bit grayscale value between 0 and 15.
 */
uint8_t rgba_to_greyscale(const RGBA &x);

/**
 * Converts the given grayscale value between 0 and 15 to an RGBA colour.
 */
RGBA greyscale_to_rgba(uint8_t g);
}  // namespace epaper_emulation
}  // namespace inktty

#endif /* INKTTY_BACKENDS_EPAPER_EMULATION_HPP */
