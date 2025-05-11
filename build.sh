#!/bin/sh

set -xe

LDFLAGS="-lGL -lglfw -lGLEW -lm"

gcc $LDFLAGS -g2 -Og src/*.c -o gterm

./gterm
