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

#include <inktty/gfx/color.hpp>

namespace inktty {

/******************************************************************************
 * Struct RGB                                                                 *
 ******************************************************************************/

const RGB RGB::Black = RGB();
const RGB RGB::White = RGB(255, 255, 255);

/******************************************************************************
 * Struct Palette                                                             *
 ******************************************************************************/

/* Ubuntu colour palette as per Wikipedia
   https://en.wikipedia.org/wiki/ANSI_escape_code */
static const RGB DEFAULT_16_COLORS[16] = {
    {1, 1, 1},       {222, 56, 43},  {57, 181, 74},  {255, 199, 6},
    {0, 111, 184},   {118, 38, 113}, {44, 181, 233}, {204, 204, 204},
    {128, 128, 128}, {255, 0, 0},    {0, 255, 0},    {255, 255, 0},
    {0, 0, 255},     {255, 0, 255},  {0, 255, 255},  {255, 255, 255}};

const Palette Palette::Default16Colours(16, DEFAULT_16_COLORS);

}  // namespace inktty
