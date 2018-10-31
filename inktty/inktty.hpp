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
 * @file inktty.hpp
 *
 * The Inktty class implements the main application.
 *
 * @author Andreas Stöckel
 */

#ifndef INKTTY_INKTTY_HPP
#define INKTTY_INKTTY_HPP

#include <memory>

#include <inktty/events/events.hpp>
#include <inktty/gfx/display.hpp>

namespace inktty {

class Inktty {
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;

public:
	/**
	 * Creates a new Inktty instance.
	 *
	 * @param event_sources is a list of external event sources.
	 * @param display is the target display.
	 */
	Inktty(const std::vector<EventSource *> &event_sources, Display &display);

	~Inktty();

	void run();
};

}  // namespace inktty

#endif /* INKTTY_INKTTY_HPP */
