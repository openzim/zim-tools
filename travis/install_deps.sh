#!/usr/bin/env bash

set -e

REPO_NAME=${TRAVIS_REPO_SLUG#*/}

# Ninja
cd $HOME
if [[ "$TRAVIS_OS_NAME" == "osx" ]]
then
  pip3 install meson

  wget https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-mac.zip
  unzip ninja-mac.zip ninja
  ARCHIVE_NAME=deps_osx_${PLATFORM}_${REPO_NAME}.tar.xz
else
  wget https://bootstrap.pypa.io/get-pip.py
  python3.5 get-pip.py --user

  python3.5 -m pip install --user --upgrade pip
  python3.5 -m pip install --user meson

  wget https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-linux.zip
  unzip ninja-linux.zip ninja
  ARCHIVE_NAME=deps_linux_xenial_${PLATFORM}_${REPO_NAME}.tar.xz
fi

mkdir -p $HOME/bin
cp ninja $HOME/bin

# Dependencies comming from kiwix-build.
cd ${HOME}
wget http://tmp.kiwix.org/ci/${ARCHIVE_NAME}
tar xf ${HOME}/${ARCHIVE_NAME}
sudo ln -s travis ../ci_builder
