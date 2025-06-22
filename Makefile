# Makefile for Yuzu Bazel Project

# Define common Bazel command
BAZEL := bazel
CC 	:= clang++-18

# Default target: build the main library
all: 
	CC=$(CC) $(BAZEL) build //...
	$(MAKE) compdb

# Target to generate compile_commands.json for IDEs (e.g., VS Code)
# This will also build the necessary C++ targets as its dependencies.
compdb:
	CC=$(CC) $(BAZEL) run @hedron_compile_commands//:refresh_all

# Target to clean Bazel's cache thoroughly
clean:
	CC=$(CC) $(BAZEL) clean --expunge

