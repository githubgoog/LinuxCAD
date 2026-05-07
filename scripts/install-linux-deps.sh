#!/usr/bin/env bash
# Install typical build dependencies for LinuxCAD on Debian/Ubuntu.
# Most users do not need this — they should download a LinuxCAD AppImage
# from GitHub Releases. This script is for developers building from source.
# Run with: bash scripts/install-linux-deps.sh

set -euo pipefail

sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
  build-essential cmake ninja-build ccache git swig pkg-config \
  qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools \
  qt6-svg-dev qt6-webengine-dev libqt6opengl6-dev libqt6openglwidgets6 \
  libxerces-c-dev libboost-all-dev libeigen3-dev libpyside6-dev \
  libshiboken6-dev pyside6-tools libcoin-dev libpcl-dev libgts-dev \
  libocct-foundation-dev \
  libocct-data-exchange-dev \
  libocct-modeling-algorithms-dev \
  libocct-modeling-data-dev \
  libocct-ocaf-dev \
  libocct-visualization-dev \
  libmedc-dev libvtk9-dev libgmsh-dev libfmt-dev libyaml-cpp-dev \
  libzipios++-dev python3-dev python3-pip python3-matplotlib \
  python3-pyside6.qtcore python3-pyside6.qtgui python3-pyside6.qtwidgets

echo "Done. Build with: ./build/build-linux.sh --install"
