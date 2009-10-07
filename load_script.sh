#!/bin/bash

servers="root@solo1 root@solo2"
function load() {
    echo Load script to $(echo $1 | awk -F@ '{print $2}')
    scp -q lua/module_*.lua  $1:/opt/tester/lua
    echo Restart tester on $(echo $1 | awk -F@ '{print $2}')
    ssh $1 'killall tester'

}
if [ $# == 0 ];then
    for server in $servers;do 
	load $server
    done
else
    load $1
fi
echo Done