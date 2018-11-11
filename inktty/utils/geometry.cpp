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

#include <inktty/utils/geometry.hpp>

namespace inktty {
/******************************************************************************
 * Class Rect                                                                 *
 ******************************************************************************/

Rect Rect::rotate(const Rect &bounds, int orientation) const {
	return Rect{};
}

/******************************************************************************
 * Class Point                                                                *
 ******************************************************************************/

Point Point::rotate(const Rect &bounds, int orientation) const {
	return Point{};
}

}  // namespace inktty