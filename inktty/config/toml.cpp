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

#include <istream>
#include <ostream>
#include <cstdint>

#include <cpptoml.h>

#include <inktty/config/toml.hpp>

namespace inktty {
namespace config {

template<typename T, typename S = T>
static void get(const char *key, std::shared_ptr<cpptoml::table> tbl, S &tar) {
	auto option = tbl->get_as<T>(key);
	if (option) {
		tar = S(*option);
	}
}

static Colors parse_colors(std::shared_ptr<cpptoml::table> tbl) {
	Colors res;
	get<bool>("use_bright_on_bold", tbl, res.use_bright_on_bold);
	get<int64_t, RGBA>("default_bg", tbl, res.default_bg);
	get<int64_t, RGBA>("default_fg", tbl, res.default_fg);
	if (tbl->contains("palette")) {
		auto arr = tbl->get_array_of<int64_t>("palette");
		if (arr) {
			for (size_t i = 0; i < arr->size(); i++) {
				res.palette[i] = RGBA((*arr)[i]);
			}
		}
	}
	return res;
}

Configuration from_toml(std::istream &is) {
	// Try to read the configuration
	auto config = cpptoml::parser(is).parse();

	// Write the configuration keys to the Configuration object
	Configuration res;
	if (config->contains("colors")) {
		res.colors = parse_colors(config->get_table("colors"));
	}

	return res;
}

}  // namespace config
}  // namespace inktty

