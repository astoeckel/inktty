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

#include <poll.h>

#include <inktty/term/events.hpp>

namespace inktty {

/******************************************************************************
 * Class Event                                                                *
 ******************************************************************************/

int Event::wait(const std::vector<EventSource *> &sources, Event &event,
                int last_source, int timeout) {
	// Make sure "last_source + 1" is in the range between [0, fds.size()]
	last_source = std::max(0, std::min<int>(sources.size(), last_source + 1));

	std::vector<struct pollfd> fds;
	std::vector<size_t> srcs;
	for (size_t i = 0; i < sources.size(); i++) {
		// Fetch the event source pointer
		const EventSource *src = sources[i];

		// Initialize the fds structure
		struct pollfd fd;
		fd.fd = src->event_fd();
		fd.events = 0;
		fd.revents = 0;
		if (fd.fd < 0) {
			continue;
		}

		// Write the requested events
		const EventSource::PollMode mode = src->event_fd_poll_mode();
		if (mode & EventSource::PollIn) {
			fd.events |= POLLIN;
		}
		if (mode & EventSource::PollOut) {
			fd.events |= POLLOUT;
		}

		fds.push_back(fd);
		srcs.push_back(i);
	}

	// Wait for events on the specified list of fds
	poll(&fds[0], fds.size(), timeout);

	// Fetch the events from the event sources.
	// TODO: handle last_source correctly
	for (size_t i = 0, j = 0; i < sources.size(); i++, j++) {
		if (srcs[j] > i) {
			continue;
		}

		if (fds[j].revents & POLLOUT) {
			if (sources[i]->event_get(EventSource::PollOut, event)) {
				return i;
			}
		} else if (fds[j].revents & POLLIN) {
			if (sources[i]->event_get(EventSource::PollIn, event)) {
				return i;
			}
		} else if (fds[j].revents & POLLNVAL || fds[j].revents & POLLHUP) {
			if (sources[i]->event_get(EventSource::PollErr, event)) {
				return i;
			}
		}
	}
	return -1;
}

}  // namespace inktty

