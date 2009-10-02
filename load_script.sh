#!/bin/bash

servers="root@solo1 root@solo2"
function load() {
    scp lua/module_*.lua  $1:/opt/tester/lua
    ssh $1 'killall tester'

}
if [ $# == 0 ];then
    for server in $servers;do 
	load $server
    done
else
    load $1
fi
