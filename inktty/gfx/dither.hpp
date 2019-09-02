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

#ifndef INKTTY_GFX_DITHER_HPP
#define INKTTY_GFX_DITHER_HPP

#include <cstddef>
#include <cstdint>

#include <inktty/utils/color.hpp>

namespace inktty {
namespace dither {
/**
 * Fills the memory region indicated by tar, tar_stride, tar_x0, tar_y0, tar_x1,
 * tar_y1 with a binary ordered dithering pattern to represent a greyscale value
 * between 0 and 16.
 *
 * @param g is the greyscale value between 0 and 15 that should be used to
 * select the dithering pattern.
 * @param tar is a pointer at the RGBA memory region to which the pattern should
 * be written.
 * @param tar_stride is the distance between two consecutive rows in memory in
 * bytes.
 * @param tar_x0, tar_y0, tar_x1, tar_y1 specify the target rectangle in pixels.
 */
void ordered_binary_4bit_greyscale(uint8_t g, RGBA *tar, size_t tar_stride,
                                   int tar_x0, int tar_y0, int tar_x1,
                                   int tar_y1);
}  // namespace dither
}  // namespace inktty

#endif /* INKTTY_GFX_DITHER_HPP */
