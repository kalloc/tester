#!/bin/bash

(sed 's|\(export PREFIX=\).\+|\1/opt/tester|' -i Makefile && 
echo -n ', Make' && make >/dev/null 2>&1 && 
echo -n ', Install' && make install >/dev/null 2>&1 && 
echo -n ', Prepare' && 
ln -fs /opt/tester/bin/luajit-2.0.0-beta5 /opt/tester/bin/luajit && 
ln -fs /opt/tester/bin/luajit-2.0.0-beta5 /opt/tester/bin/lua &&
ln -sf /opt/tester/include/luajit-2.0 /opt/tester/include/lua &&
ln -sf /opt/tester/lib/libluajit-5.1.a  /opt/tester/lib/liblua.a 
) || (echo 'ERROR INSTALL LuaJIT Lib' ;exit)
