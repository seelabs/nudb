#!/usr/bin/env bash

# Assumptions:
# 1) BOOST_ROOT and BOOST_URL are already defined,
# and contain valid values.
# 2) The last namepart of BOOST_ROOT matches the
# folder name internal to boost's .tar.gz

set -eu

if [[ -d ${BOOST_ROOT}/lib ]]; then
  echo "Using cached boost at $BOOST_ROOT"
  exit 0
fi

wget ${BOOST_URL} -O /tmp/boost.tar.gz
pushd $(dirname $BOOST_ROOT) > /dev/null
rm -fr ${BOOST_ROOT}
tar xzf /tmp/boost.tar.gz
popd > /dev/null

pushd ${BOOST_ROOT} > /dev/null
./bootstrap.sh --prefix=$BOOST_ROOT
params="address-model=${DDRESS_MODEL}"
./b2 $params
./b2 -d0 $params install
popd > /dev/null

