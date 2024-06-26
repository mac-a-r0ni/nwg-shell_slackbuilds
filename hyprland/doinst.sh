# putting this here because it's a hyprland specific issue
# hyprland will link against libdisplay-info 0.2.0 but unless it's symlinked
# to the old lib, it fails to start
( cd usr/lib64 ; ln -sf libdisplay-info.so.0.2.0 libdisplay-info.so.1 )

