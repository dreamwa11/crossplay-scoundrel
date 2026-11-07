#!/bin/sh
# Builds and runs the Scoundrel rules tests. No device and no PlatformIO:
# ScoundrelCore is freestanding C++17.
#
#   host-tests/scoundrel/run.sh
set -e
cd "$(dirname "$0")"
BUILD_DIR="${TMPDIR:-/tmp}/$(basename "${CXX:-c++}")-scoundrel-tests-$(cd ../.. && pwd | cksum | cut -d" " -f1)"
mkdir -p "$BUILD_DIR"
SRC=../../src/apps_local/scoundrel
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -O2 -I$SRC \
  test_scoundrel.cpp $SRC/ScoundrelCore.cpp -o "$BUILD_DIR/test_scoundrel"
"$BUILD_DIR/test_scoundrel" "$@"
