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
    default_options : ['c_std=c99', 'cpp_std=c++11', 'b_ndebug=if-release'])

# Dependencies
dep_freetype = dependency('freetype2', required: true)
dep_vtparse = dependency('vtparse',
	fallback:['vtparse', 'dep_vtparse'],
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

# Main library
inc_inktty = include_directories('.')
lib_inktty = library(
	'inktty',
	[
		'inktty/gfx/color.cpp',
		'inktty/gfx/display.cpp',
		'inktty/gfx/font.cpp',
		'inktty/gfx/matrix.cpp',
		'inktty/term/events.cpp',
		'inktty/term/pty.cpp',
		'inktty/term/vt100.cpp',
		'inktty/utils/geometry.cpp',
		'inktty/utils/utf8.cpp',
		'inktty/inktty.cpp',
	],
	include_directories: [inc_inktty, inc_utf8proc],
	dependencies: [dep_freetype, dep_vtparse],
	link_with: [lib_utf8proc]
)

# Unit tests
dep_foxenunit = dependency(
    'libfoxenunit',
    fallback:['libfoxenunit', 'dep_foxenunit'])
exe_test_utils_utf8 = executable(
    'test_utils_utf8',
    'test/utils/test_utf8.cpp',
    include_directories: [inc_inktty],
    dependencies: [dep_foxenunit],
    link_with: [lib_inktty],
    install: false)
test('test_utils_utf8', exe_test_utils_utf8)

# SDL
dep_threads = dependency('threads')
dep_sdl = dependency('sdl2', required: false)
if dep_sdl.found()
	exe_inktty_sdl = executable(
		'inktty_sdl',
		[
			'inktty/backends/sdl.cpp',
			'inktty/backends/kbdstdin.cpp',
			'inktty/main_sdl.cpp',
		],
		include_directories: inc_inktty,
		link_with: [lib_inktty],
		dependencies: [dep_sdl, dep_threads]
	)
endif

# Framebuffer
exe_inktty_fbdev = executable(
	'inktty_fbdev',
	[
		'inktty/backends/fbdev.cpp',
		'inktty/backends/kbdstdin.cpp',
		'inktty/main_fbdev.cpp',
	],
	include_directories: inc_inktty,
	link_with: [lib_inktty],
)

