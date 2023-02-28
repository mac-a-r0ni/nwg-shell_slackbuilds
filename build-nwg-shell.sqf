# This file is an attempt to see if nwg-shell will build on 15.0 with SBo packages
# If using this script with sbopkg, do NOT also use the build-nwg-shell.sh you will
# make a mess, and I'm not fixing it.

# Basic dependencies for building the shell

google-go-lang
papirus-icon-theme
scdoc
xdg-desktop-portal-wlr
libhandy
gtk-layer-shell
SwayNotificationCenter
nlohmann_json
light
seatd
wlroots
swaybg
sway

# Python3 dependencies
# you will find no consistency with naming of python
# packages on SBo which stresses me out considerably
#

wheel
python3-installer
python3-pep517
python3-attrs
python3-multidict
yarl
frozenlist
aiosignal
python3-build
python3-flit_core
typing-extensions
async-timeout
python3-aiohttp
python3-autotiling
python3-xlib
i3ipc
python3-dasbus
geographiclib-python
geopy
python3-psutil
python3-netifaces

#
# Build the rest of nwg-shell & dependencies
#

imlib2
feh
glm
slop
maim
send2trash
wlr-randr
azote
tllist
fcft
foot
gopsuinfo
grim
jq
lxappearance
slurp
swayidle
swaylock
scour
# submit
wdisplays
wl-clipboard
# submit ALL
wlsunset
nwg-icon-picker
nwg-bar
nwg-displays
nwg-dock
nwg-drawer
nwg-launchers
nwg-look
nwg-menu
nwg-panel
nwg-shell
nwg-shell-config
nwg-shell-wallpapers
nwg-wrapper
