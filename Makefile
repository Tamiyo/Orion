# Makefile wrapping CMake for the Yuzu project.

BUILD_DIR := build
GENERATOR := Ninja
CC        := clang
CXX       := clang++
CLANG_FORMAT := clang-format

# Hand-written C++ sources (generated *.inc live under $(BUILD_DIR), excluded).
CXX_SOURCES := $(shell find yuzu \( -name '*.cc' -o -name '*.h' \))

.PHONY: all configure build test repl compile format format-check grammar clean

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

# Compile a single file as one unit: `make compile FILE=path/to/x.yz`.
# Extra flags pass through ARGS, e.g. `make compile FILE=x.yz ARGS=--debug-hir`.
compile: build
	$(BUILD_DIR)/yuzu/tools/compile/yuzu-compile $(ARGS) $(FILE)

# Format all C++ sources in place using .clang-format.
format:
	$(CLANG_FORMAT) -i $(CXX_SOURCES)

# Report files that aren't formatted (non-zero exit if any), without editing.
format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(CXX_SOURCES)

# Regenerate the VS Code TextMate grammar from the lexer's TokenKind.td.
# Builds only yuzu-tblgen, so it works even while the rest of the tree is
# mid-change.
grammar: configure
	cmake --build $(BUILD_DIR) --target yuzu-tblgen
	$(BUILD_DIR)/yuzu/tools/tblgen/yuzu-tblgen -gen-textmate-grammar \
		-I yuzu/include yuzu/include/yuzu/Lexer/TokenKind.td \
		> editors/vscode/syntaxes/yuzu.tmLanguage.json

clean:
	rm -rf $(BUILD_DIR)
