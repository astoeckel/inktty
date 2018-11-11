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

#ifndef INKTTY_TERM_EVENTS_HPP
#define INKTTY_TERM_EVENTS_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace inktty {

/* Forward declaration */
struct EventSource;

struct Event {
	/**
	 * Number of bytes that can be stored in the data buffer.
	 */
	static constexpr size_t BUF_SIZE = 1024;

	enum class Type {
		/**
		 * No event happened.
		 */
		NONE,

		KEYBD_KEY_DOWN,

		KEYBD_KEY_UP,

		/**
		 * Text has been read from the keyboard.
		 */
		TEXT_INPUT,

		/**
		 * A mouse button was pressed.
		 */
		MOUSE_BTN_DOWN,

		/**
		 * A mouse button was released.
		 */
		MOUSE_BTN_UP,

		/**
		 * The mouse was moved.
		 */
		MOUSE_MOVE,

		/**
		 * A mouse click was received.
		 */
		MOUSE_CLICK,

		/**
		 * The terminal is supposed to exit.
		 */
		QUIT,

		/**
		 * The terminal window was resized.
		 */
		RESIZE,

		/**
		 * Output was received from the child
		 */
		CHILD_OUTPUT
	};

	struct Keyboard {
		/**
		 * If the key corresponds to a unicode character, codepoint should be
		 * set to this character.
		 */
		uint32_t codepoint;

		/**
		 * Untranslated keycode.
		 */
		uint32_t scancode;

		/**
		 * True if the shift key is pressed at the same time.
		 */
		bool shift : 1;

		/**
		 * True if the control key is pressed at the same time.
		 */
		bool ctrl : 1;

		/**
		 * True if the alt key is pressed at the same time.
		 */
		bool alt : 1;

		/**
		 * True if the key is a printable character. In this case, the
		 * "codepoint" should be set.
		 */
		bool is_char : 1;
	};

	struct Mouse {
		enum Button { none = 0, left = 1, middle = 2, right = 4 };

		/**
		 * X- and Y-coordinate of the mouse cursor.
		 */
		int x, y;

		/**
		 * Button that triggered the event (if any).
		 */
		Button trigger;

		/**
		 * Buttons currently being pressed.
		 */
		Button state;
	};

	struct Child {
		/**
		 * Number of bytes received from the child process.
		 */
		size_t buf_len;

		/**
		 * Data received from the child process.
		 */
		uint8_t buf[BUF_SIZE];
	};

	struct Text {
		/**
		 * Number of bytes received from the child process.
		 */
		size_t buf_len;

		/**
		 * Data received from the child process.
		 */
		uint8_t buf[BUF_SIZE];
	};

	Type type = Type::NONE;
	union Data {
		Keyboard keybd;
		Mouse mouse;
		Child child;
		Text text;
	} data;

	/**
	 * Waits for a single new within the given list of event sources.
	 *
	 * @param sources is a vector listing all available EventSource instances.
	 * @param event is a reference at a Event instance in which the event should
	 * be stored.
	 * @param last_source is a pointer at the index of the last event source
	 * that produced an event. This value is used to fairly round-robin between
	 * event sources. It should be set to the last values returned by this
	 * function. A negative value indicates that there was no last event. This
	 * value is assumed to be -1 if the pointer is set to NULL.
	 * @param timeout is a timeout in milliseconds. If the timeout expires
	 * without any event happening in the meantime this function returns -1.
	 * @return the
	 */
	static int wait(const std::vector<EventSource *> &sources, Event &event,
	                int last_source = -1, int timeout = -1);
};

/**
 * The EventSource class is an interface that should be implemented by all
 * event providers. Each event source must have an associated file-descriptor
 * that can be polled for incoming events using the poll() system call.
 */
struct EventSource {
	/**
	 * Poll mode that should be used to read from the device. Values may be
	 * combined via bit-wise OR.
	 */
	enum PollMode { PollNone = 0, PollIn = 1, PollOut = 2, PollErr = 4 };

	/**
	 * Virtual destructor.
	 */
	virtual ~EventSource(){};

	/**
	 * Returns a file descriptor that can be polled.
	 */
	virtual int event_fd() const = 0;

	/**
	 * Returns the mode that should be used to poll the file descriptor.
	 */
	virtual PollMode event_fd_poll_mode() const = 0;

	/**
	 * Reads the next event and writes it to the given event descriptor.
	 */
	virtual bool event_get(PollMode mode, Event &event) = 0;
};
}  // namespace inktty

#endif /* INKTTY_TERM_EVENTS_HPP */
