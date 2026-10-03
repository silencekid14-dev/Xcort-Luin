#!/usr/bin/env bash
# Builds Luin for Linux (static libgcc/libstdc++ where possible).
# Requires raylib installed system-wide (headers + libraylib) and X11/GL dev packages.
set -euo pipefail
cd "$(dirname "$0")"

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -I/usr/local/include -Ithird_party/raylib/include"
LDFLAGS="-L/usr/local/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -static-libgcc -static-libstdc++"

# NOTE: do NOT add src/ via -I. Files such as math.h and time.h in src/ would
# shadow the system <math.h>/<time.h>. They are found via "quoted" includes only.
SOURCES=(
  main.cpp AST.cpp Lexer.cpp Token.cpp Parser.cpp Interpreter.cpp
  math.cpp time.cpp string_module.cpp random_module.cpp arrays_module.cpp
  os_module.cpp app_module.cpp gui.cpp pkg_module.cpp sxc.cpp version.cpp
)
SRC_PATHS=("${SOURCES[@]/#/src/}")

mkdir -p bin/linux
echo "Compiling Luin ..."
$CXX $CXXFLAGS -o bin/linux/luin "${SRC_PATHS[@]}" $LDFLAGS
echo "Built ./bin/linux/luin"
ls -l bin/linux/luin
