#!/usr/bin/env bash

set -euxo pipefail
# The above bash options do the following:

# -e When this option is on, if a simple command fails for any of the reasons
#    listed in Consequences of Shell Errors or returns an exit status value >0,
#    and is not part of the compound list following a while, until, or if
#    keyword, and is not a part of an AND or OR list, and is not a pipeline
#    preceded by the ! reserved word, then the shell shall immediately exit.
# -u The shell shall write a message to standard error when it tries to expand a
#    variable that is not set and immediately exit. An interactive shell shall
#    not exit.
# -x The shell shall write to standard error a trace for each command after it
#    expands the command and before it executes it. It is unspecified
#    whether the command that turns tracing off is traced.
# -o pipefail
#    Pipelines fail on the first command which fails instead of dying later on
#    down the pipeline.

# Put an appropriate gcc symlink at the front of the path.
mkdir ${HOME}/bin
for g in gcc g++ gcov gcc-ar gcc-nm gcc-ranlib
do
  g_path = $(type -p ${g}-${GCC_VER})
  if [[ -x ${g_path} ]]; then
      ln -sv ${g_path} $HOME/bin/${g}
  else
      echo "Could not find ${g}-${GCC_VER}"
      exit 1
  fi
done

if [[ -n ${CLANG_VER:-} ]]; then
    # There are cases where the directory exists, but the exe is not available.
    # Use this workaround for now.
    if [[ ! -x llvm-${LLVM_VERSION}/bin/llvm-config ]] && [[ -d llvm-${LLVM_VERSION} ]]; then
        rm -fr llvm-${LLVM_VERSION}
    fi
    if [[ ! -d llvm-${LLVM_VERSION} ]]; then
        mkdir llvm-${LLVM_VERSION}
        LLVM_URL="http://llvm.org/releases/${LLVM_VERSION}/clang+llvm-${LLVM_VERSION}-x86_64-linux-gnu-ubuntu-14.04.tar.xz"
        wget -O - ${LLVM_URL} | tar -Jxvf - --strip 1 -C llvm-${LLVM_VERSION}
    fi
    llvm-${LLVM_VERSION}/bin/llvm-config --version;
    export LLVM_CONFIG="llvm-${LLVM_VERSION}/bin/llvm-config";
fi

export PATH=${HOME}/bin:$PATH

[[ -x $HOME/bin/g++ ]] && ${HOME}/bin/g++ -v
[[ -x $HOME/bin/clang ]] && ${HOME}/bin/clang -v

bash scripts/install-boost.sh
bash scripts/install-valgrind.sh

# Install lcov
# Download the archive
wget http://downloads.sourceforge.net/ltp/lcov-1.12.tar.gz
# Extract to ~/lcov-1.12
tar xfvz lcov-1.12.tar.gz -C ${HOME}
# Set install path
mkdir -p ${LCOV_ROOT}

pushd ${HOME}/lcov-1.12 > /dev/null
make install PREFIX=${LCOV_ROOT}
popd > /dev/null
