#  inktty -- Terminal emulator optimized for epaper displays
#  Copyright (C) 2018  Andreas Stöckel
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.

project('inktty', ['cpp', 'c'],
    default_options : [
        'c_std=c99',
        'cpp_std=c++11',
        'b_ndebug=if-release',
        'buildtype=release',
        'default_library=static'])

# Dependencies
dep_freetype = dependency('freetype2', required: false)
dep_sdl = dependency('sdl2', required: false)
dep_threads = dependency('threads')
dep_vterm = dependency('vterm',
	fallback:['vterm', 'dep_vterm'],
	required: true)
dep_termkey = dependency('termkey',
	fallback:['termkey', 'dep_termkey'],
	required: true)

# utf8proc library
inc_utf8proc = include_directories('./lib')
lib_utf8proc = library(
	'utf8proc',
	[
		'lib/utf8proc/utf8proc.c',
	],
	include_directories: [inc_utf8proc]
)

# cpptoml dependencies
inc_cpptoml = include_directories('./lib/cpptoml/include')
inc_mxcfb = include_directories('./lib')

conf_data = configuration_data()
conf_data.set('HAS_FREETYPE', dep_freetype.found())
conf_data.set('HAS_SDL', dep_sdl.found())
configure_file(input : 'config.h.in',
               output : 'config.h',
               configuration : conf_data)

# Main library
inc_inktty = include_directories('.')
lib_inktty = library(
	'inktty',
	[
		'inktty/backends/fbdev.cpp',
		'inktty/backends/kbdstdin.cpp',
		'inktty/backends/sdl.cpp',
		'inktty/config/argparse.cpp',
		'inktty/config/configuration.cpp',
		'inktty/config/toml.cpp',
		'inktty/fontdata/font_8x16.c',
		'inktty/gfx/display.cpp',
		'inktty/gfx/dither.cpp',
		'inktty/gfx/epaper_emulation.cpp',
		'inktty/gfx/font.cpp',
		'inktty/gfx/font_bitmap.cpp',
		'inktty/gfx/font_cache.cpp',
		'inktty/gfx/font_ttf.cpp',
		'inktty/gfx/matrix_renderer.cpp',
		'inktty/term/events.cpp',
		'inktty/term/matrix.cpp',
		'inktty/term/pty.cpp',
		'inktty/term/vterm.cpp',
		'inktty/utils/ansi_terminal_writer.cpp',
		'inktty/utils/color.cpp',
		'inktty/utils/geometry.cpp',
		'inktty/utils/logger.cpp',
		'inktty/utils/utf8.cpp',
		'inktty/inktty.cpp',
	],
	include_directories: [inc_inktty, inc_utf8proc, inc_cpptoml, inc_mxcfb],
	dependencies: [dep_freetype, dep_vterm, dep_termkey, dep_sdl, dep_threads],
	link_with: [lib_utf8proc]
)

# Unit tests
dep_foxenunit = dependency(
    'libfoxenunit',
    fallback:['libfoxenunit', 'dep_foxenunit'])
exe_test_utils_color = executable(
    'test_utils_color',
    'test/utils/test_color.cpp',
    include_directories: [inc_inktty],
    dependencies: [dep_foxenunit],
    link_with: [lib_inktty],
    install: false)
exe_test_utils_utf8 = executable(
    'test_utils_utf8',
    'test/utils/test_utf8.cpp',
    include_directories: [inc_inktty],
    dependencies: [dep_foxenunit],
    link_with: [lib_inktty],
    install: false)
exe_test_term_matrix = executable(
    'test_term_matrix',
    'test/term/test_matrix.cpp',
    include_directories: [inc_inktty],
    dependencies: [dep_foxenunit],
    link_with: [lib_inktty],
    install: false)
test('test_utils_color', exe_test_utils_color)
test('test_utils_utf8', exe_test_utils_utf8)
test('test_term_matrix', exe_test_term_matrix)

# Framebuffer
exe_inktty = executable(
	'inktty',
	[
		'inktty/main.cpp',
	],
	include_directories: [inc_inktty],
	link_with: [lib_inktty],
)

