#!/bin/bash
# reconfigures autotools setup
# using rjf's laptop PATH settings.
set -x
autoreconf -i
./configure CLANG_DEV_LIBS=/opt/homebrew/opt/llvm
make
