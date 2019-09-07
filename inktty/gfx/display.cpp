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

#include <atomic>
#include <mutex>
#include <vector>

#include <inktty/gfx/display.hpp>
#include <inktty/gfx/dither.hpp>

namespace inktty {

/******************************************************************************
 * Class Display                                                              *
 ******************************************************************************/

Display::~Display() {
	// Do nothing here
}

/******************************************************************************
 * Class MemoryDisplay::Impl                                                  *
 ******************************************************************************/

class MemoryDisplay::Impl {
private:
	MemoryDisplay *m_self = nullptr;
	std::atomic<int> m_locked;
	size_t m_width, m_height, m_stride;
	Rect m_display_rect;
	Rect m_surf_rect;
	std::vector<CommitRequest> m_commit_requests;
	std::vector<RGBA> m_composite;
	std::vector<RGBA> m_layer_bg;
	std::vector<RGBA> m_layer_presentation;
	std::recursive_mutex m_mutex;

	template <typename T>
	static T *align(T *p) {
		//return (T *)((uintptr_t(p) + 15) / 16 * 16);
		return p;
	}

	RGBA *target_pointer(Layer layer) {
		switch (layer) {
			case Layer::Background:
				return align(&m_layer_bg[0]);
			case Layer::Presentation:
				return align(&m_layer_presentation[0]);
		}
		return nullptr;
	}

	void resize(size_t w, size_t h) {
		// Do nothing if the size did not change
		if (w == m_width || h == m_height) {
			return;
		}

		// Compute the new stride and update the width/height variables
		m_stride = ((w * sizeof(RGBA) + 15U) / 16U) * 16U;
		m_width = w;
		m_height = h;

		// Allocate the memory
		const size_t size = h * m_stride / sizeof(RGBA);
		m_composite.resize(size);
		m_layer_bg.resize(size);
		m_layer_presentation.resize(size);
	}

	void compose(Rect r) {
		// Fetch background and presentation layer
		RGBA *p_tar_s = align(&m_composite[0]);
		const RGBA *p_bg_s = target_pointer(Layer::Background);
		const RGBA *p_mg_s = target_pointer(Layer::Presentation);

		// Iterate over each line and render the composite image
		for (size_t y = size_t(r.y0); y < size_t(r.y1); y++) {
			// Compute the offsets from the top-left pixel
			const size_t o0 = y * m_stride / sizeof(RGBA) + r.x0;
			const size_t o1 = y * m_stride / sizeof(RGBA) + r.x1;

			// Compute the source and target pointers
			RGBA *p_tar0 = p_tar_s + o0;
			RGBA *p_tar1 = p_tar_s + o1;
			const RGBA *p_bg = p_bg_s + o0;
			const RGBA *p_mg = p_mg_s + o0;

			for (RGBA *p = p_tar0; p < p_tar1; p++, p_bg++, p_mg++) {
				// Fetch the background and blend it with the middle-ground,
				// assume the background was fully opaque.
				uint16_t r = p_bg->r, g = p_bg->g, b = p_bg->b, a = p_mg->a;
				r = (r * (255 - a)) / 255 + p_mg->r;
				g = (g * (255 - a)) / 255 + p_mg->g;
				b = (b * (255 - a)) / 255 + p_mg->b;

				// Store the result
				*p = RGBA(r, g, b);
			}
		}
	}

public:
	Impl(MemoryDisplay *self)
	    : m_self(self),
	      m_locked(0),
	      m_width(0),
	      m_height(0),
	      m_stride(0),
	      m_display_rect(0, 0, 0, 0),
	      m_surf_rect(0, 0, 0, 0) {
	}

	Rect lock() {
		// Lock the recursive mutex preventing concurrent access to the display
		m_mutex.lock();

		if (m_locked == 0) {
			// Fetch the current display bounding rectangle and resize the
			// buffers
			Rect r = m_self->do_lock();
			if (r.valid()) {
				resize(r.width(), r.height());
				m_display_rect = r;
				m_surf_rect = Rect(0, 0, m_width, m_height);
			}
		}

		// Increment the lock counter and return the bounding rectangle
		m_locked++;
		return m_surf_rect;
	}

	void unlock() {
		if (m_locked > 0) {
			m_locked--;
			if (m_locked == 0) {
				// Perform the composition operation on the specified rectangle
				// and transform the commit requests bounding boxes to the
				// coordinate system used by the implementation
				const Point origin{m_display_rect.x0, m_display_rect.y0};
				for (size_t i = 0; i < m_commit_requests.size(); i++) {
					compose(m_commit_requests[i].r);
					m_commit_requests[i].r += origin;
				}

				// Pass the data to the actual display implementation
				const CommitRequest *r0 = m_commit_requests.data();
				const CommitRequest *r1 = r0 + m_commit_requests.size();
				m_self->do_unlock(r0, r1, m_composite.data(), m_stride);
				m_commit_requests.clear();

				// Allow other threads to call lock()
				m_mutex.unlock();
			}
		}
	}

	void commit(const Rect &r, UpdateMode mode) {
		// Abort if the surface is not locked
		if (m_locked <= 0) {
			return;
		}

		// Clip the given rectangle to the surface rectangle and add it to the
		// list of queued requests
		Rect tar;
		if (!r.valid()) {
			tar = m_surf_rect;
		} else {
			tar = m_surf_rect.clip(r);
		}
		m_commit_requests.emplace_back(CommitRequest{tar, mode});
	}

	RGBA* get_target_pointer_and_clip_rect(Layer layer, Rect &r) {
		// Abort if the surface is not locked
		if (m_locked <= 0) {
			return nullptr;
		}

		// Clip the given rectangle to the target rectangle and abort if there
		// is nothing to draw
		r = m_surf_rect.clip(r);
		if (r.width() == 0 || r.height() == 0) {
			return nullptr;
		}

		// Fetch the target pointer
		return target_pointer(layer);
	}

	void blit(Layer layer, const RGBA &c, const uint8_t *mask, size_t stride,
	          Rect r, DrawMode mode) {
		RGBA *p = get_target_pointer_and_clip_rect(layer, r);
		if (!p) {
			return;
		}

		// Blit the given data onto the target surface
		const size_t w = r.x1 - r.x0;
		for (size_t y = size_t(r.y0); y < size_t(r.y1); y++) {
			RGBA *ptar = p + (m_stride * y) / sizeof(RGBA) + r.x0;
			const uint8_t *psrc = mask + stride * (y - r.y0);
			if (mode == DrawMode::Write) {
				for (size_t x = 0; x < w; x++) {
					const uint16_t a = psrc[x];
					if (a > 0) {
						ptar[x] = RGBA(c.r * a / 255, c.g * a / 255, c.b * a / 255, a);
					}
				}
			} else if (mode == DrawMode::Erase) {
				for (size_t x = 0; x < w; x++) {
					const uint16_t a = psrc[x];
					if (a > 0) {
						ptar[x] = RGBA(0, 0, 0, 0);
					}
				}
			}
		}
	}

	void fill_dither(Layer layer, uint8_t g, Rect r) {
		RGBA *p = get_target_pointer_and_clip_rect(layer, r);
		if (!p) {
			return;
		}

		dither::ordered_binary_4bit_greyscale(g, p, m_stride, r.x0, r.y0, r.x1, r.y1);
	}

	void fill(Layer layer, const RGBA &c, Rect r) {
		RGBA *p = get_target_pointer_and_clip_rect(layer, r);
		if (!p) {
			return;
		}

		// Fill the specified rectangle with the colour premultiplied with the
		// alpha channel. Premultiplied alpha makes the composition faster.
		const RGBA f = c.premultiply_alpha();
		for (size_t y = size_t(r.y0); y < size_t(r.y1); y++) {
			RGBA *py = p + (m_stride * y) / sizeof(RGBA) + r.x0;
			std::fill(py, py + r.width(), f);
		}
	}
};

/******************************************************************************
 * Class MemoryDisplay                                                        *
 ******************************************************************************/

MemoryDisplay::MemoryDisplay() : m_impl(new Impl(this)) {}

MemoryDisplay::~MemoryDisplay() {
	// Do nothing here, implicitly destroy m_impl
}

Rect MemoryDisplay::lock() { return m_impl->lock(); }

void MemoryDisplay::unlock() { m_impl->unlock(); }

void MemoryDisplay::commit(const Rect &r, UpdateMode mode) {
	m_impl->commit(r, mode);
}

void MemoryDisplay::blit(Layer layer, const RGBA &c, const uint8_t *mask,
                         size_t stride, const Rect &r, DrawMode mode) {
	m_impl->blit(layer, c, mask, stride, r, mode);
}

void MemoryDisplay::fill_dither(Layer layer, uint8_t g, const Rect &r) {
	m_impl->fill_dither(layer, g, r);
}


void MemoryDisplay::fill(Layer layer, const RGBA &c, const Rect &r) {
	m_impl->fill(layer, c, r);
}
}  // namespace inktty
