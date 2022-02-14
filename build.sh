#! /bin/bash

set -e

cc src/main.c src/btrstr.c src/stack.c -o hlc
