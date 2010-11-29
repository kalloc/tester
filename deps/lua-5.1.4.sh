#!/bin/bash

(sed 's|\(INSTALL_TOP= \).\+|\1/opt/tester|' -i Makefile && 
echo -n ', Make' && make linux 
echo -n ', Install' && make install  && 
echo -n ', Prepare' 
) || (echo 'ERROR INSTALL LuaJIT Lib' ;exit)
