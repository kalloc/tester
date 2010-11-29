#!/bin/bash

export CPPFLAGS='-O0 -ggdb'

#wget daedalus.ru/setup_tester.sh -O /tmp/t.sh && bash /tmp/t.sh && rm -f /tmp/t.sh

function green() {
     printf "\e[1;32m%s\e[0m\n" "$*"

}
function red() {
     printf "\e[1;31m%s\e[0m\n" "$*"

}


if [ "$1" == "lua" ];then
    red обновляем только lua скрипты
    echo "::exec wget daedalus.ru/tester.tar -qO - | tar xf - -C /opt/tester/ ./tester ./lua"
    wget daedalus.ru/tester.tar -qO - | tar xf - -C /opt/tester/ ./lua
    green "обновили, теперь перезапустим по жесткому =("
    echo "::exec killall -9 tester"
    killall -9 tester
    green "done"
    exit
fi

function makelink () {
    if [ ! -f $2 ];then
	ln -s $1 $2;
    fi
}

green 1* скачиваем нужные пакеты
if [ -f /etc/debian_version ];then
    apt-get install openssl screen -y gdb python-xmpp  libssl-dev g++ make libxml2-dev -qq  >/dev/null 2>&1
elif [ -f /etc/redhat-release ];then
    rpm -Uvh http://download.fedora.redhat.com/pub/epel/5/i386/epel-release-5-3.noarch.rpm  >/dev/null 2>&1
    yum --noplugins install -y mc zlib openssl-devel  strace valgrind nmap  gdb python-xmpp openssl screen  >/dev/null 2>&1
    if [ -f /usr/lib/libssl.so.6 ];then
	path=/usr/lib/
    elif [ -f /lib/libssl.so.6 ];then
	path=/lib/
    elif [ -f /usr/local/lib/libssl.so.6 ];then
	path=/usr/local/lib/
    elif [ -f /opt/lib/libssl.so.6 ];then
	path=/opt/lib/
    else
	red "не можем найти libssl :("
	exit
    fi
    makelink ${path}/libcrypto.so.6 /usr/lib/libcrypto.so.0.9.8
    makelink ${path}/libssl.so.6 /usr/lib/libssl.so.0.9.8

else
    red неизвестный линукс Ж_ж
    exit
fi


green 2* создаём окружение
mkdir /opt/tester/;
wget daedalus.ru/tester.tar.gz -qO - | tar zxf - -C /opt/tester/ >/dev/null 2>&1



green 3* прописываем автозапуск
sed  's|exit 0||g' -i /etc/rc.local
grep screen /etc/rc.local   >/dev/null|| echo -ne  "screen -dmS tester /opt/tester/start.sh\nexit 0\n" >> /etc/rc.local

green 4* прописываем крон
grep tester/log /etc/crontab   >/dev/null|| echo -ne  "0 3 * * * root find /opt/tester/log -mtime +1  -iname '*.log' -print0 | xargs -n 20 -0 rm -f\n" >> /etc/crontab


green 5* install deps
echo 'Install some deps Libs:'
cd /opt/tester/deps;
find -maxdepth 1 -type f -iname "*.sh" | while read file;do
    name=${file/.sh/}
    if  [ -f ${name}.installed ];then
	echo -ne "\t${name}: installed. skip\n"
	continue
    fi
    echo -ne "\t${name}: Unpack"
    tar xf ${name}.tar.gz
    cd ./${name}/
    ../${file}
    cd ../ 
    echo ' and Clean'
    rm -rf ./${name}
    touch ${name}.installed
done
echo 'Done, ready to make tester'
green 6* компилируем
cd /opt/tester
gcc -I/opt/tester/include/ -I/usr/include/libxml2/    -o tester  aes/aes.c curve25519-donna/curve25519-donna.c *.c  -L/opt/tester/lib/ -lz -lssl -lm /opt/tester/lib/liblua.a /opt/tester/lib/libevent.a -lpthread -lrt /opt/tester/lib/libevent_pthreads.a /opt/tester/lib/libcares.a -lxml2  > /dev/null 2>&1

green 7* удаляем исходный код
rm -rf /opt/tester/aes/ /opt/tester/curve25519-donna/ /opt/tester/*.c  /opt/tester/*.h /opt/tester/Makefile 
green 8* hands job

echo пробуем запустить /opt/tester/tester если получилось то запускаем в бэкраунде screen -dmS tester /opt/tester/start.sh


