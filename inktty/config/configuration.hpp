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
 * This file contains the structure holding information about the current
 * terminal setup.
 */

#ifndef INKTTY_CONFIG_CONFIGURATION_HPP
#define INKTTY_CONFIG_CONFIGURATION_HPP

#include <string>

#include <inktty/utils/color.hpp>

namespace inktty {
namespace config {

/**
 * General configuration options, such as the name of the backend that should be
 * used.
 */
struct General {
	/**
	 * Backend to use, may be one of "sdl" or "fbdev".
	 */
	std::string backend;

	/**
	 * Initial orientation. Should be in the range 0-3.
	 */
	int orientation;

	/**
	 * Default constructor, sets all values to defaults.
	 */
	General();
};

/**
 * Contains configuration options regarding the colors used in the terminal.
 */
struct Colors {
	/**
	 * If true, uses bright colours for bold test. Default is false.
	 */
	bool use_bright_on_bold;

	/**
	 * The default background colour, i.e. if no other background colour is set,
	 * this colour will be used.
	 */
	RGBA default_bg;

	/**
	 * The default foreground colour, i.e. if no other colour is set, this
	 * colour will be used as a text colour.
	 */
	RGBA default_fg;

	/**
	 * A palette defining the appearance of the 256 colours. This is initialized
	 * to the default 256 colour palette. When loaded from the configuration
	 * file entries will only be overwritten, the palette will not be replaced,
	 * i.e., specifying the first 16 colours will only overwrite the first 16
	 * colours.
	 */
	Palette palette;

	/**
	 * Default constructor, initializes all values to default values.
	 */
	Colors();
};

}  // namespace config

/**
 * The Configuration class contains all the configuration sub-objects. It also
 * provides functions to load the configuration from a given filename or to
 * parse the given command line arguments.
 */
struct Configuration {
	/**
	 * General configuration options.
	 */
	config::General general;

	/**
	 * Object containing all color configuration options.
	 */
	config::Colors colors;

	/**
	 * Initialises the configuration to default values and does nothing.
	 */
	Configuration();

	/**
	 * Parses the given command line options. The application will exit if there
	 * was an error parsing the configuration, or if the user requested the
	 * help.
	 */
	Configuration(int argc, const char *argv[]);

	/**
	 * Reads the configuration from the specified TOML file. Applies POSIX word
	 * expansion to the given filename.
	 */
	Configuration(const char *filename);
};

}  // namespace inktty

#endif /* INKTTY_CONFIG_CONFIGURATION_HPP */
