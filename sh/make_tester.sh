#!/bin/bash
cd ../
build=$(date +%Y_%m_%d-%H_%M)
tar zcf dist/tester_${build}.tar.gz ./aes ./curve25519-donna ./deps ./lua ./docs ./sh  Makefile *.c *.py *.h *.xml
ln -sf tester_${build}.tar.gz dist/tester.tar.gz
cd sh
