# Makefile wrapping CMake for the Yuzu project.

BUILD_DIR := build
GENERATOR := Ninja
CC        := clang
CXX       := clang++

.PHONY: all configure build test repl clean

all: build

configure:
	cmake -S . -B $(BUILD_DIR) -G $(GENERATOR) \
		-DCMAKE_C_COMPILER=$(CC) \
		-DCMAKE_CXX_COMPILER=$(CXX) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

build: configure
	cmake --build $(BUILD_DIR)

test: build
ifdef TEST
	ctest --test-dir $(BUILD_DIR) --output-on-failure -R $(TEST)
else
	ctest --test-dir $(BUILD_DIR) --output-on-failure
endif

# Build yuzu-repl (and only what it depends on) and launch it interactively.
# The `repl` CMake target is `USES_TERMINAL`, so the running binary gets
# the real tty for stdin/stdout.
repl: configure
	cmake --build $(BUILD_DIR) --target repl

clean:
	rm -rf $(BUILD_DIR)
