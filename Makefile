.PHONY: configure build test install clean distclean

PRESET ?= dev

all: build

configure:
	cmake --preset $(PRESET)

build: configure
	cmake --build --preset $(PRESET)

test: build
	ctest --preset $(PRESET)

install: build
	cmake --build --preset $(PRESET) --target install

clean:
	@if [ -d build ]; then cmake --build --preset $(PRESET) --target clean; fi

distclean:
	rm -rf build
