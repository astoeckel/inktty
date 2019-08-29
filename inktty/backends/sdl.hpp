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

#ifndef INKTTY_BACKENDS_SDL_HPP
#define INKTTY_BACKENDS_SDL_HPP

#include <config.h>

#ifdef HAS_SDL

#include <memory>

#include <inktty/gfx/display.hpp>
#include <inktty/term/events.hpp>

namespace inktty {
/**
 * The SDLBackend class implements both a display and event source backend for
 * Inktty.
 */
class SDLBackend : public MemoryDisplay, public EventSource {
private:
	/**
	 * Impl is the actual private implementation of the SDLBackend class.
	 */
	class Impl;
	std::unique_ptr<Impl> m_impl;

protected:
	/* Implementation of the abstract class MemoryDisplay */
	Rect do_lock() override;
	void do_unlock(const CommitRequest *begin, const CommitRequest *end,
	               const RGBA *buf, size_t stride) override;

public:
	/**
	 * Creates a new SDLDisplay instance that opens a window with the specified
	 * width and height.
	 */
	SDLBackend(unsigned int width, unsigned int height);

	/**
	 * Destroys the SDLDisplay instance.
	 */
	~SDLBackend();

	/* Implementation of the abstract class EventSource */
	int event_fd() const override;
	EventSource::PollMode event_fd_poll_mode() const override;
	bool event_get(EventSource::PollMode mode, Event &event) override;
};
}  // namespace inktty

#endif /* HAS_SDL */
#endif /* INKTTY_BACKENDS_SDL_HPP */
