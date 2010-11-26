#!/bin/bash


(echo -n ', Configure' && ./configure --prefix=/opt/tester >/dev/null 2>&1  &&
 echo -n ', Make' &&  make >/dev/null 2>&1 && 
 echo -n ', Install' && make install >/dev/null 2>&1 ) || (echo 'ERROR INSTALL C-Ares Lib';exit)
