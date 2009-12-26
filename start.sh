#!/bin/bash 
ulimit -c unlimited
ulimit -n 99999
path="/opt/tester"
cmd="tester"

cd $path

if [ ! -d  $path/core ];then   mkdir $path/core;fi
#вечный цикл
while [ 1 ];do
    dir=$(date "+%Y-%m-%d_%H.%M.%S")
    $path/${cmd} $path/config.xml > ${dir}.log 2>&1


    #если при падение найдена кора, то это ккбэ плохо (.
    if [ "`ls core.* 2>/dev/null`" ];then
	#уведомляем кодера о ошибке посредством электронной связи
	if [ ! -f  /tmp/.gdb.$cmd ] || [ $(( $(date +%s) - $( date +%s -r /tmp/.gdb.$cmd))) -gt 600 ];then 

	echo "bt full" > /tmp/.gdb.${cmd}
	
	dir=$(date "+%Y-%m-%d_%H.%M.%S")
	$path/wakeupmylord.py nickname@ainmarh.com "
my lord, ${cmd} now crashed :(
lucky news, i'm try to restart (if crash repeatly you be flooded ;)
----------------------[stacktrace]----------------------------
$(gdb -batch -x /tmp/.gdb.${cmd} $path/${cmd} $path/core.*)
--------------------------------------------------------------
"


	fi
	#архивируем кору
	cat ${path}/log/* | tail -n 5000 > ${path}/last.log
	tar zcf ${path}/core/${dir}.tar.gz ./last.log ./core.* ./${cmd}  -C ${path}/
	#зачищаем
	rm -rf ${path}/core.* ${path}/log/*.log ${path}/last.log 
    fi
done
