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

#include <iostream>
#include <memory>
#include <stdexcept>

#include <inktty/backends/fbdev.hpp>
#include <inktty/backends/kbdstdin.hpp>
#include <inktty/backends/sdl.hpp>
#include <inktty/config/configuration.hpp>
#include <inktty/inktty.hpp>

using namespace inktty;

static std::unique_ptr<Display> get_display(const std::string &name, std::vector<EventSource *> &event_sources) {
	std::unique_ptr<Display> display;
#ifdef HAS_SDL
	if ((name == "sdl" || name == "default") && !display) {
		try {
			SDLBackend *sdl_backend = new SDLBackend(800, 600);
			display = std::unique_ptr<Display>(sdl_backend);
			event_sources.push_back(sdl_backend);
		} catch (std::runtime_error &e) {
			std::cerr << "WARNING: Coudln't open SDL backend: " << e.what()
			          << std::endl;
		}
	}
#endif
	if ((name == "fbdev" || name == "default") && !display) {
		try {
			display = std::unique_ptr<Display>(new FbDevDisplay("/dev/fb0"));
		} catch (std::runtime_error &e) {
			std::cerr << "WARNING: Coudln't open framebuffer backend: "
			          << e.what() << std::endl;
		}
	}
	return display;
}

int main(int argc, const char *argv[]) {
	// Load the configuration
	Configuration config(argc, argv);

	// Try to allocate a display
	std::vector<EventSource*> event_sources;
	std::unique_ptr<Display> display = get_display(config.general.backend, event_sources);
	if (!display) {
		std::cerr << "FATAL: Couldn't allocate a display." << std::endl;
		return 1;
	}

	// If there is no event source yet, append the terminal keyboard
	std::unique_ptr<KbdStdin> keyboard;
	if (event_sources.empty()) {
		keyboard = std::unique_ptr<KbdStdin>(new KbdStdin());
		event_sources.push_back(keyboard.get());
	}

	Inktty(config, event_sources, *display).run();
	return 0;
}
