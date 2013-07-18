#!/bin/bash

if test -n "${INSTALL_PKG_SESSION_ID}"
then
  # darwin installation from package manager
  export HOMEPATH=${2}
else
  export HOMEPATH=${PWD}/rst
fi

mkdir -p ${HOMEPATH}/script
cp -v ${HOMEPATH}/codebase/superdarn/src.script/os/* ${HOMEPATH}/script




