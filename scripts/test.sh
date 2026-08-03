#!/usr/bin/env sh
set -eu
mkdir -p build
${CC:-gcc} -std=c11 -O0 -g -Wall -Wextra -Wpedantic -Wconversion -Iinclude \
  tests/test_repository.c src/instrument.c src/input.c src/repository.c \
  -o build/lab-manager-tests
./build/lab-manager-tests
