#!/bin/bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
[[ $(dpkg --print-architecture) == amd64 ]] || { echo 'Only amd64 is targeted.' >&2; exit 1; }
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build-release
ctest --test-dir build-release --output-on-failure
(cd build-release && cpack -G DEB)
