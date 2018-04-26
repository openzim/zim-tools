#!/usr/bin/env bash

set -e

BUILD_DIR=${HOME}/BUILD_${PLATFORM}
INSTALL_DIR=${BUILD_DIR}/INSTALL


case ${PLATFORM} in
  "native_static")
    MESON_OPTION="-Dstatic-linkage=true"
    ;;
  "native_dyn")
    MESON_OPTION=""
    ;;
esac

cd ${TRAVIS_BUILD_DIR}
if [[ "${TRAVIS_OS_NAME}" == "osx" ]]
then
  export PKG_CONFIG_PATH=${INSTALL_DIR}/lib/pkgconfig
else
  export PKG_CONFIG_PATH=${INSTALL_DIR}/lib/x86_64-linux-gnu/pkgconfig
fi
meson . build ${MESON_OPTION}
cd build
ninja
