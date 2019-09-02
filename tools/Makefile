CXX?=g++ # Use g++ if no other compiler is specified

CPPFLAGS+=-std=c++11
CPPFLAGS+=-O3
CPPFLAGS+=-Wno-psabi # Silence warning when compiling for ARM
CPPFLAGS+=-Wall -Wextra -pedantic
CPPFLAGS+=-I..

all: mxcfb_test_update_modes epaper_emulation

mxcfb_test_update_modes: main_mxcfb_test_update_modes.cpp
	$(CXX) $(CPPFLAGS) -o mxcfb_test_update_modes -lpthread \
		main_mxcfb_test_update_modes.cpp

epaper_emulation: main_epaper_emulation.cpp
	$(CXX) $(CPPFLAGS) -o epaper_emulation \
		main_epaper_emulation.cpp

clean:
	rm -f mxcfb_test_update_modes epaper_emulation
