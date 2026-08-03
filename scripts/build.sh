#!/usr/bin/env sh
set -eu
mkdir -p build
${CC:-gcc} -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Iinclude \
  src/main.c src/cli.c src/instrument.c src/input.c src/repository.c \
  -o build/lab-manager
echo "Built build/lab-manager"
