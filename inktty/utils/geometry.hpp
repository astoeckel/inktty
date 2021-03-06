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
 * @file geometry.hpp
 *
 * This file contains utilities for basic geometry processing.
 *
 * @author Andreas Stöckel
 */

#ifndef INKTTY_UTILS_GEOMETRY_HPP
#define INKTTY_UTILS_GEOMETRY_HPP

#include <memory>
#include <algorithm>
#include <limits>

namespace inktty {

struct Rect;

struct Point {
	int x, y;

	constexpr Point() : x(0), y(0) {}

	constexpr Point(int x, int y) : x(x), y(y) {}

	friend Point operator +(const Point &p, const Point &q) {
		return Point(p.x + q.x, p.y + q.y);
	}

	friend Point operator -(const Point &p, const Point &q) {
		return Point(p.x - q.x, p.y - q.y);
	}

	Point& operator+=(const Point &q) {
		x += q.x, y += q.y;
		return *this;
	}

	Point& operator-=(const Point &q) {
		x -= q.x, y -= q.y;
		return *this;
	}
};

struct Rect {
	int x0, y0, x1, y1;

	/**
	 * Creates a new, invalid rectangle.
	 */
	constexpr Rect()
	    : x0(std::numeric_limits<int>::max()),
	      y0(std::numeric_limits<int>::max()),
	      x1(std::numeric_limits<int>::min()),
	      y1(std::numeric_limits<int>::min()) {}

	/**
	 * Creates a new rectangle with the given boundary points. x0, y0
	 * corresponds to the upper-left point of the rectangle, x1, y1 to the
	 * lower-right point of the rectangle.
	 */
	constexpr Rect(int x0, int y0, int x1, int y1)
	    : x0(x0), y0(y0), x1(x1), y1(y1) {}

	constexpr static Rect sized(int x, int y, int w, int h) {
		return Rect(x, y, x + w, y + h);
	}

	/**
	 * Returns true if the rectangle is valid.
	 */
	constexpr bool valid() const { return (x0 <= x1) && (y0 <= y1); }

	/**
	 * Returns the width of the rectangle. This operation only makes sense if
	 * the rectangle is valid.
	 */
	constexpr int width() const { return x1 - x0; }

	/**
	 * Returns the height of the rectangle. This operation only makes sense if
	 * the rectangle is valid.
	 */
	constexpr int height() const { return y1 - y0; }

	constexpr int area() const { return width() * height(); }

	/**
	 * Clips the given x-coordinate to a valid x-coordinate within the screen
	 * region.
	 */
	int clipx(int x, bool border = false) const {
		if (border) {
			return (x < x0) ? x0 : ((x > x1) ? x1 : x);
		} else {
			return (x < x0) ? x0 : ((x >= x1) ? (x1 - 1) : x);
		}
	}

	/**
	 * Clips the given y-coordinate to a valid y-coordinate within the screen
	 * region.
	 */
	int clipy(int y, bool border = false) const {
		if (border) {
			return (y < y0) ? y0 : ((y > y1) ? y1 : y);
		} else {
			return (y < y0) ? y0 : ((y >= y1) ? (y1 - 1) : y);
		}
	}

	Point clip(const Point &p, bool border = false) const {
		return Point(clipx(p.x, border), clipy(p.y, border));
	}

	Rect clip(const Rect &r) const {
		return Rect{clipx(r.x0), clipy(r.y0), clipx(r.x1, true),
		            clipy(r.y1, true)};
	}

	Rect grow(const Rect &r) const {
		return Rect{std::min(x0, r.x0), std::min(y0, r.y0), std::max(x1, r.x1),
		            std::max(y1, r.y1)};
	}

	Rect grow(const Point &p) const {
		return Rect{std::min(x0, p.x), std::min(y0, p.y), std::max(x1, p.x),
		            std::max(y1, p.y)};
	}

	Rect &operator+=(const Point &p) {
		x0 += p.x, y0 += p.y, x1 += p.x, y1 += p.y;
		return *this;
	}

	friend Rect operator+(const Rect &r, const Point &p) {
		const int x0 = r.x0 + p.x, y0 = r.y0 + p.y;
		const int x1 = r.x1 + p.x, y1 = r.y1 + p.y;
		return Rect(x0, y0, x1, y1);
	}

	bool operator==(const Rect &r) const {
		return (x0 == r.x0) && (y0 == r.y0) && (x1 == r.x1) && (y1 == r.y1);
	}

	bool operator!=(const Rect &r) const {
		return (x0 != r.x0) || (y0 != r.y0) || (x1 != r.x1) || (y1 != r.y1);
	}
};

/**
 * The RectangleMerger class tries to merge multiple overlapping or close
 * rectangles into larger rectangles without increasing the total area covered
 * by the larger rectangles by too much.
 */
class RectangleMerger {
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;

public:
	RectangleMerger();
	~RectangleMerger();

	/**
	 * Resets the rectangle merger to its initial state.
	 */
	void reset();

	/**
	 * Inserts a new rectangle into the rectangle merger and tries to combine
	 * it with the available rectangles.
	 */
	void insert(const Rect &r);

	/**
	 * Tries to merge individual rectangles currently stored in the rectangle
	 * merger.
	 */
	void merge();

	const Rect* begin() const;
	const Rect* end() const;
};

}  // namespace inktty

#endif /* INKTTY_UTILS_GEOMETRY_HPP */
