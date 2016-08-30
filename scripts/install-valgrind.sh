#!/usr/bin/env bash

# Assumptions:
# 1) VALGRIND_ROOT is already defined, and contains a valid values

set -eu
if [[ ! -d ${VALGRIND_ROOT}/bin ]]; then
   echo "Using cached valgrind at $VALGRIND_ROOT"
   exit 0
fi

# These are specified in the addons/apt section of .travis.yml
# sudo apt-get install subversion automake autotools-dev libc6-dbg
export PATH=${PATH}:${VALGRIND_ROOT}/bin
svn co svn://svn.valgrind.org/valgrind/trunk valgrind-co

pushd valgrind-co > /dev/null

./autogen.sh
./configure --prefix=${VALGRIND_ROOT}
make
make install

valgrind ls -l

popd > /dev/null
