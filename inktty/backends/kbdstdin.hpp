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

#ifndef INKTTY_BACKENDS_KBDSTDIN_HPP
#define INKTTY_BACKENDS_KBDSTDIN_HPP

#include <memory>

#include <inktty/term/events.hpp>

namespace inktty {
/**
 * Event source listening on stdin for keyboard events.
 */
class KbdStdin : public EventSource {
private:
	struct Data;
	std::unique_ptr<Data> m_data;

public:
	KbdStdin(int fd_stdin=0, int fd_stdout=1);
	~KbdStdin();

	/* Implementation of the abstract class EventSource */
	int event_fd() const override;
	EventSource::PollMode event_fd_poll_mode() const override;
	bool event_get(EventSource::PollMode mode, Event &event) override;
};
}  // namespace inktty

#endif /* INKTTY_BACKENDS_KBDSTDIN_HPP */
