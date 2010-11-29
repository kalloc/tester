#!/bin/bash
export CPPFLAGS='-O0 -ggdb'
echo 'Install some deps Libs:'
cd ./deps;
find -maxdepth 1 -type f -iname "*.sh" | while read file;do

    name=${file/.sh/}
    echo -ne "\t${name}: Unpack"
    tar xf ${name}.tar.gz
    cd ./${name}/
    ../${file}
    cd ../ 
    echo ' and Clean'
    rm -rf ./${name}
done
echo 'Done, ready to make tester'