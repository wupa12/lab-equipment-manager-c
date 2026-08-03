CC ?= gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion
CPPFLAGS ?= -Iinclude

CORE_SOURCES = src/instrument.c src/input.c src/repository.c
APP_SOURCES = src/main.c src/cli.c $(CORE_SOURCES)
TEST_SOURCES = tests/test_repository.c $(CORE_SOURCES)

.PHONY: all test clean

all: build/lab-manager

build:
	mkdir -p build

build/lab-manager: $(APP_SOURCES) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(APP_SOURCES) -o $@

build/lab-manager-tests: $(TEST_SOURCES) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_SOURCES) -o $@

test: build/lab-manager-tests
	./build/lab-manager-tests

clean:
	rm -rf build
