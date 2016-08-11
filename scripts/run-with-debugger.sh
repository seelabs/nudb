#!/usr/bin/env bash

set -eu

if [[ $(uname) == Darwin ]]; then
  # -o runs after loading the binary
  # -k runs after any crash
  # use script to exit with 1 on crash (like gdb's --return-child-result)
  lldb --batch \
       -o 'run' \
       -k 'thread backtrace all' \
       -k 'script import os; os._exit(1)' \
       $@
else
  gdb --silent \
      --batch \
      --return-child-result \
      -ex="set print thread-events off" \
      -ex=run \
      -ex="thread apply all bt full" \
      --args $@
fi
