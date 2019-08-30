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
 *
 */

#include <fstream>
#include <iostream>

#include <unistd.h>
#include <wordexp.h>

#include <inktty/config/argparse.hpp>
#include <inktty/config/configuration.hpp>
#include <inktty/config/toml.hpp>
#include <inktty/utils/logger.hpp>

namespace inktty {
namespace config {

/******************************************************************************
 * Class WordExp                                                              *
 ******************************************************************************/

namespace {
/**
 * Used internally to expand filenames according to POSIX con
 */
class WordExp {
private:
	wordexp_t m_exp;

public:
	WordExp(const char *str) {
		if (wordexp(str, &m_exp, WRDE_SHOWERR) != 0) {
			exit(EXIT_FAILURE);
		}
	}

	~WordExp() { wordfree(&m_exp); }

	size_t size() const { return m_exp.we_wordc; }

	const char *operator[](size_t i) const { return m_exp.we_wordv[i]; }
};

}  // namespace

/******************************************************************************
 * Class General                                                              *
 ******************************************************************************/

General::General() : orientation(0) {}

/******************************************************************************
 * Class Colors                                                               *
 ******************************************************************************/

Colors::Colors()
    : use_bright_on_bold(false),
      default_bg(RGBA::Black),
      default_fg(RGBA(170, 170, 170)),
      palette(Palette::Default256Colours) {
	// Load the nicer default 16 colours into the palette
	for (size_t i = 0; i < 16; i++) {
		palette[i] = Palette::Default16Colours[i];
	}
}

}  // namespace config

/******************************************************************************
 * Class Configuration                                                        *
 ******************************************************************************/

using namespace config;

Configuration::Configuration() {}

Configuration::Configuration(int argc, const char *argv[]) {
	Argparse argparse(
	    "inktty",
	    "A terminal emulator (not only) optimized for epaper displays");
	argparse.add_arg(
	    "config", "Location of the configuration file.", nullptr,
	    [this](const char *value) -> bool {
		    *this = Configuration(value);
		    return true;
	    },
	    Argparse::Required::NOT_REQUIRED);
	argparse.add_arg(
	    "backend", "Backend to use.", "default",
	    [this](const char *value) -> bool {
		    this->general.backend = value;
		    return true;
	    },
	    Argparse::Required::NOT_REQUIRED);
	argparse.parse(argc, argv);
}

Configuration::Configuration(const char *filename) {
	WordExp filename_exp(filename);
	std::ifstream is(filename);
	if (!is) {
		global_logger().warn()
		    << "Configuration file \"" << filename << "\" not found.";
	} else {
		*this = from_toml(is);
	}
}

}  // namespace inktty

