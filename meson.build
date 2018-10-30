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

project('inktty', 'cpp',
    default_options : ['cpp_std=c++11', 'b_ndebug=if-release'])

# Include directory
inc_inktty = include_directories('.')

# Freetype as a dependency
dep_freetype = dependency('freetype2', required: true)

# Main library
lib_inktty = library(
	'inktty',
	[
		'inktty/gfx/color.cpp',
		'inktty/gfx/display.cpp',
		'inktty/gfx/font.cpp',
		'inktty/gfx/matrix.cpp',
		'inktty/term/pty.cpp',
	],
	include_directories: inc_inktty,
	dependencies: [dep_freetype]
)

# SDL
dep_sdl = dependency('sdl2', required: false)
if dep_sdl.found()
	exe_inktty_sdl = executable(
		'inktty_sdl',
		[
			'inktty/backends/sdl.cpp',
			'inktty/main_sdl.cpp'
		],
		include_directories: inc_inktty,
		link_with: [lib_inktty],
		dependencies: [dep_sdl]
	)
endif

# Framebuffer
exe_inktty_fbdev = executable(
	'inktty_fbdev',
	[
		'inktty/backends/fbdev.cpp',
		'inktty/main_fbdev.cpp'
	],
	include_directories: inc_inktty,
	link_with: [lib_inktty],
)

