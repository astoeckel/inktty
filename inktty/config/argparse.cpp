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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include <inktty/config/argparse.hpp>

namespace inktty {
namespace config {

/******************************************************************************
 * Class Argparse::Impl                                                       *
 ******************************************************************************/

class Argparse::Impl {
private:
	struct Arg {
		const char *name;
		char switch_char;
		const char *descr;
		const char *default_;
		Argparse::Callback cback;
		bool is_switch;
		Argparse::Required required;
	};

	const char *m_prog_name;
	const char *m_prog_descr;
	std::vector<Arg> m_args;

public:
	Impl(const char *prog_name, const char *prog_descr)
	    : m_prog_name(prog_name), m_prog_descr(prog_descr) {
		add(
		    "help", 'h', "Displays this message and exits", nullptr,
		    [this](const char *) -> bool {
			    print_usage();
			    exit(EXIT_FAILURE);
			    return false;
		    },
		    true, Required::NOT_REQUIRED);
	}

	void add(const char *name, char switch_char, const char *descr,
	         const char *default_, const Argparse::Callback &cback,
	         bool is_switch, Argparse::Required required) {
		m_args.emplace_back(Arg{name, switch_char, descr, default_, cback,
		                        is_switch, required});
	}

	void print_usage() const {
		std::cout << m_prog_descr << std::endl << std::endl;
		std::cout << "Usage: " << m_prog_name;
		for (const Arg &arg : m_args) {
			if (arg.is_switch) {
				std::cout << " [--" << arg.name;
				if (arg.switch_char) {
					std::cout << ","
					          << "-" << arg.switch_char;
				}
				std::cout << "]";
			} else {
				if (arg.default_) {
					std::cout << " [--" << arg.name << "=" << arg.default_
					          << "]";
				} else {
					std::cout << " --" << arg.name << " <VALUE>";
				}
			}
		}
		std::cout << std::endl << std::endl;

		std::cout << "Where the arguments have the following meaning:"
		          << std::endl;
		for (const Arg &arg : m_args) {
			std::cout << "\t--" << arg.name;
			if (arg.switch_char) {
				std::cout << ", "
				          << "-" << arg.switch_char;
			}
			std::cout << std::endl;
			std::cout << "\t\t" << arg.descr << std::endl;
		}
		std::cout << std::endl;
	}

	void parse(int argc, const char *argv[]) const {
		// Determine which args are required and keep track of all arguments
		// that have been specified by the user.
		std::vector<bool> required_args(m_args.size(), false);
		std::vector<bool> specified_args(m_args.size(), false);
		for (size_t i = 0; i < m_args.size(); i++) {
			switch (m_args[i].required) {
				case Required::REQUIRED:
					required_args[i] = true;
					break;
				case Required::NOT_REQUIRED:
					required_args[i] = false;
					break;
				case Required::AUTO:
					required_args[i] = (m_args[i].default_ == nullptr) &&
					                   (!m_args[i].is_switch);
					break;
			}
		}

		// Try to parse the arguments
		for (int i = 1; i < argc; i++) {
			char const *arg = argv[i];
			bool valid = false;
			if (arg[0] == '\0') {
				continue;
			} else if (arg[0] == '-' && arg[1] == '-') {
				arg = &arg[2];
				for (size_t j = 0; j < m_args.size(); j++) {
					if (strcmp(m_args[j].name, arg) == 0) {
						if (specified_args[j]) {
							std::cerr << "\"" << argv[i]
							          << "\" specified multiple times."
							          << std::endl;
							exit(EXIT_FAILURE);
						}
						specified_args[j] = true;

						char const *value = nullptr;
						if (!m_args[j].is_switch) {
							if (i + 1 < argc) {
								value = argv[++i];
							} else {
								std::cerr << "Expected value for \"" << argv[i]
								          << "\"" << std::endl;
								exit(EXIT_FAILURE);
							}
						}

						if (!(m_args[j].cback(value))) {
							if (value) {
								std::cerr << "Error while parsing argument \""
								          << argv[i] << "=" << value << "\""
								          << std::endl;
							} else {
								std::cerr << "Error while parsing switch \""
								          << argv[i] << "\"" << std::endl;
							}
							exit(EXIT_FAILURE);
						}

						valid = true;
						break;
					}
				}
			} else if (arg[0] == '-') {
				valid = true;
				for (size_t k = 1; arg[k]; k++) {
					bool valid = false;
					for (size_t j = 0; j < m_args.size(); j++) {
						if (m_args[j].switch_char != arg[k]) {
							continue;
						}

						if (specified_args[j]) {
							std::cerr << "\"" << argv[i]
							          << "\" specified multiple times."
							          << std::endl;
							exit(EXIT_FAILURE);
						}
						specified_args[j] = true;

						if (!(m_args[j].cback(nullptr))) {
							std::cerr << "Error while parsing switch \""
							          << argv[i] << "\"" << std::endl;
							exit(EXIT_FAILURE);
						}
						valid = true;
						break;
					}
					if (!valid) {
						std::cerr << "Unkown switch \"" << arg[k] << "\""
						          << std::endl;
						exit(EXIT_FAILURE);
					}
				}
			}
			if (!valid) {
				std::cerr << "Expected argument but got \"" << argv[i] << "\""
				          << std::endl;
				exit(EXIT_FAILURE);
			}
		}

		// Make sure all required parameters have been parsed and call callbacks
		// with default parameters.
		for (size_t i = 0; i < m_args.size(); i++) {
			if (specified_args[i]) {
				continue;
			}
			if (required_args[i]) {
				std::cerr << "Required argument \"--" << m_args[i].name
				          << "\" not specified." << std::endl;
				exit(EXIT_FAILURE);
			}
			if (m_args[i].default_) {
				m_args[i].cback(m_args[i].default_);
			}
		}
	}
};

/******************************************************************************
 * Class Argparse                                                             *
 ******************************************************************************/

Argparse::Argparse(const char *prog_name, const char *prog_descr)
    : m_impl(new Impl(prog_name, prog_descr)) {}

Argparse::~Argparse() {
	// Implicitly destroy m_impl
}

Argparse &Argparse::add_arg(const char *name, const char *descr,
                            const char *default_, const Callback &cback,
                            Argparse::Required required) {
	m_impl->add(name, '\0', descr, default_, cback, false, required);
	return *this;
}

Argparse &Argparse::add_switch(const char *name, char switch_char,
                               const char *descr, const Callback &cback) {
	m_impl->add(name, switch_char, descr, nullptr, cback, true,
	            Required::NOT_REQUIRED);
	return *this;
}

void Argparse::parse(int argc, const char *argv[]) const {
	m_impl->parse(argc, argv);
}

}  // namespace config
}  // namespace inktty
