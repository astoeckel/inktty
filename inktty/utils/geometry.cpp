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
#include <vector>

#include <inktty/utils/geometry.hpp>

namespace inktty {

/******************************************************************************
 * Class RectangleMerger::Impl                                                *
 ******************************************************************************/

/**
 * The RectangleMerger class tries to merge multiple overlapping or close
 * rectangles into larger rectangles without increasing the total area covered
 * by the larger rectangles by too much.
 */
class RectangleMerger::Impl {
private:
	std::vector<Rect> m_rects;

	int search_matching_rectangle(const Rect &r, int i0, int i1) const {
		const int r_area = r.area();
		for (int i = i1 - 1; i >= 0; i--) {
			const Rect &s = m_rects[i];
			const int s_area = s.area();

			// Merge two rectangles if the percentage of the area covered by the
			// two rectangles withing the larger rectangle is larger than 3 / 4.
			const Rect rs = r.grow(s);
			if (r_area + s_area >= (3 * rs.area()) / 4) {
				return i;
			}
		}
		return -1;
	}

public:
	void reset() { m_rects.clear(); }

	void insert(const Rect &r) {
		const int idx = search_matching_rectangle(r, 0, m_rects.size());
		if (idx >= 0) {
			m_rects[idx] = m_rects[idx].grow(r);
		} else {
			m_rects.emplace_back(r);
		}
	}

	void merge() {
		// Repeat this process until no further merges can be found
		bool found_merge;
		do {
			found_merge = false;
			// Iterate backwards over the list of rectangles
			for (int i = int(m_rects.size()) - 1; i >= 1; i--) {
				// Try to find a rectangle up to the given index that matches
				// the current rectangle
				const int idx = search_matching_rectangle(m_rects[i], 0, i);

				// If we found a match, update the corresponding rectangle and
				// invalidate the currently selected rectangle
				if (idx >= 0) {
					m_rects[idx] = m_rects[idx].grow(m_rects[i]);
					m_rects[i] = Rect();
					found_merge = true;
				}
			}

			// Remove invalid rectangles from the list
			if (found_merge) {
				m_rects.erase(
				    std::remove_if(m_rects.begin(), m_rects.end(),
				                   [](const Rect &r) { return !r.valid(); }),
				    m_rects.end());
			}
		} while (found_merge);
	}

	const Rect *begin() const { return &(*m_rects.cbegin()); }

	const Rect *end() const { return &(*m_rects.cend()); }
};

/******************************************************************************
 * Class RectangleMerger                                                      *
 ******************************************************************************/

RectangleMerger::RectangleMerger() : m_impl(new RectangleMerger::Impl()) {}

RectangleMerger::~RectangleMerger() {
	// Do nothing here, make sure the m_impl destructor is called
}

void RectangleMerger::reset() { m_impl->reset(); }

void RectangleMerger::insert(const Rect &r) { m_impl->insert(r); }

void RectangleMerger::merge() { m_impl->merge(); }

const Rect *RectangleMerger::begin() const { return m_impl->begin(); }

const Rect *RectangleMerger::end() const { return m_impl->end(); }

}

