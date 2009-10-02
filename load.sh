#!/bin/bash

servers="root@solo1 root@solo2"
function load() {
    ssh $1 'rm -f /opt/tester/tester'
    scp dist/Debug/GNU-Linux-x86/tester  $1:/opt/tester
    ssh $1 'killall tester'

}
if [ $# == 0 ];then
    for server in $servers;do 
	load $server
    done
else
    load $1
fi
