#!/bin/sh

# Copyright 2012  Patrick J. Volkerding, Sebeka, Minnesota, USA
# All rights reserved.
#
# Copyright 2023 Jay Lanagan (j@lngn.net), Detroit, MI, USA.
# All rights reserved.
#
# Based on the xfce-build-all.sh script by Patrick J. Volkerding
#
# Redistribution and use of this script, with or without modification, is
# permitted provided that the following conditions are met:
#
# 1. Redistributions of this script must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
#  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
#  EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
#  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Set to 1 if you'd like to install/upgrade package as they are built.
# This is recommended.
INST=1

# This is where all the compilation and final results will be placed
TMP=${TMP:-/tmp/nwg}
OUTPUT=${OUTPUT:-/tmp}

# This is the original directory where you started this script
NWGROOT=$(pwd)

# Check for duplicate sources (default: OFF)
CHECKDUPLICATE=0

# Loop for all packages
for dir in \
  google-go-lang \
  xdg-desktop-portal-wlr \
  papirus-icon-theme \
  swaybg \
  sway \
  python-wheel \
  python-installer \
  python-pep517 \
  python-aiohttp \
  python-aiosignal \
  python-async-timeout \
  python-attrs \
  python-multidict \
  python-yarl \
  python-autotiling \
  python-frozenlist \
  python-xlib \
  python-i3ipc \
  python-build \
  python-dasbus \
  python-flit_core \
  python-geographiclib \
  python-geopy \
  python-psutil \
  python-typing_extensions \
  python-netifaces \
  SwayNotificationCenter \
  light \
  brightnessctl \
  imlib2 \
  feh \
  glm \
  slop \
  maim \
  send2trash \
  wlr-randr \
  azote \
  tllist \
  fcft \
  foot \
  gopsuinfo \
  grim \
  jq \
  lxappearance \
  slurp \
  swayidle \
  swaylock \
  wdisplays \
  wl-clipboard \
  wlsunset \
  nwg-icon-picker \
  nwg-bar \
  nwg-displays \
  nwg-dock \
  nwg-drawer \
  nwg-launchers \
  nwg-look \
  nwg-menu \
  nwg-panel \
  nwg-shell \
  nwg-shell-config \
  nwg-shell-wallpapers \
  nwg-wrapper \
  ; do
  # Get the package name
  package=$(echo $dir | cut -f2- -d /)

  # Change to package directory
  cd $NWGROOT/$dir || exit 1

  # Get the version
  version=$(cat ${package}.SlackBuild | grep "VERSION:" | head -n1 | cut -d "-" -f2 | rev | cut -c 2- | rev)

  # Get the build
  build=$(cat ${package}.SlackBuild | grep "BUILD:" | cut -d "-" -f2 | rev | cut -c 2- | rev)

  if [ $CHECKDUPLICATE -eq 1 ]; then
    # Check for duplicate sources
    sourcefile="$(ls -l $NWGROOT/$dir/${package}-*.tar.?z* | wc -l)"
    if [ $sourcefile -gt 1 ]; then
      echo "You have following duplicate sources:"
      ls $NWGROOT/$dir/${package}-*.tar.?z* | cut -d " " -f1
      echo "Please delete sources other than ${package}-$version to avoid problems"
      exit 1
    fi
  fi

  # The real build starts here
  TMP=$TMP OUTPUT=$OUTPUT sh ${package}.SlackBuild || exit 1
  if [ "$INST" = "1" ]; then
    PACKAGE=`ls $OUTPUT/${package}-${version}-*-${build}*nwg.txz`
    if [ -f "$PACKAGE" ]; then
      upgradepkg --install-new --reinstall "$PACKAGE"
    else
      echo "Error:  package to upgrade "$PACKAGE" not found in $OUTPUT"
      exit 1
    fi
  fi

  # back to original directory
  cd $NWGROOT
done
