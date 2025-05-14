#!/bin/sh

set -xe

TARGET="./termhero"

LDFLAGS="-lGL -lglfw -lGLEW -lm"

gcc $LDFLAGS -O3 src/*.c -o $TARGET

$TARGET
