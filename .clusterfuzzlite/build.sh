#!/bin/bash -eu
# Copy all fuzzer executables to $OUT/
$CC $CFLAGS $LIB_FUZZING_ENGINE \
  $SRC/mjson/.clusterfuzzlite/mjson_fuzzer.c \
  -o $OUT/mjson_fuzzer \
  $SRC/mjson/src/mjson.c -I$SRC/mjson/src/
