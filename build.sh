#!/usr/bin/env bash
# Builds Luin_v2.3 (Linux/macOS).
# Produces: ./Luin_v2.3
set -euo pipefail
cd "$(dirname "$0")"

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter"

SOURCES=(
  main.cpp
  AST.cpp
  Lexer.cpp
  Token.cpp
  Parser.cpp
  Interpreter.cpp
  math.cpp
  time.cpp
  string_module.cpp
  random_module.cpp
  arrays_module.cpp
  os_module.cpp
  app_module.cpp
  sxc.cpp
)

echo "Compiling Luin_v2.3 ..."
$CXX $CXXFLAGS -o Luin_v2.3 "${SOURCES[@]}"
echo "Built ./Luin_v2.3"
ls -l Luin_v2.3
