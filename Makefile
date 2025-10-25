# Makefile for Yuzu Bazel Project

# Define common Bazel command
BAZEL := bazel
CC := clang++-18

.PHONY: all build test test_single compdb clean

all:
	$(MAKE) build
	$(MAKE) compdb

build: build-tools build-lib build-test

build-tools:
	CC=$(CC) $(BAZEL) build //yuzu/tools/...

build-lib:
	CC=$(CC) $(BAZEL) build //yuzu/lib/...

build-test:
	CC=$(CC) $(BAZEL) build //yuzu/test/...

test:
ifdef TEST
	CC=$(CC) $(BAZEL) test //... --test_arg=--gtest_filter=$(TEST)
else
	CC=$(CC) $(BAZEL) test //...
endif

compdb:
	CC=$(CC) $(BAZEL) run @hedron_compile_commands//:refresh_all

clean:
	CC=$(CC) $(BAZEL) clean --expunge
	rm compile_commands.json

