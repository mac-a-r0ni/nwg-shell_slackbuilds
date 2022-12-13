( cd usr/lib64 ; rm -rf libgtk-layer-shell.so )
( cd usr/lib64 ; ln -sf libgtk-layer-shell.so.0 libgtk-layer-shell.so )
( cd usr/lib64 ; rm -rf libgtk-layer-shell.so.0 )
( cd usr/lib64 ; ln -sf libgtk-layer-shell.so.0.7.0 libgtk-layer-shell.so.0 )
