CXX?=g++ # Use g++ if no other compiler is specified

CPPFLAGS+=-std=c++11
CPPFLAGS+=-O3
CPPFLAGS+=-Wno-psabi # Silence warning when compiling for ARM
CPPFLAGS+=-Wall -Wextra -pedantic
CPPFLAGS+=-I.

all: mxcfb_test_update_modes

mxcfb_test_update_modes: main_mxcfb_test_update_modes.cpp
	$(CXX) $(CPPFLAGS) -o mxcfb_test_update_modes -lpthread \
		main_mxcfb_test_update_modes.cpp

clean:
	rm -f mxcfb_test_update_modes
