#!/bin/bash

#wget daedalus.ru/setup_tester.sh -O /tmp/t.sh && bash /tmp/t.sh && rm -f /tmp/t.sh

function green() {
     printf "\e[1;32m%s\e[0m\n" "$*"

}
function red() {
     printf "\e[1;31m%s\e[0m\n" "$*"

}


if [ -d /opt/tester ];then
    red тестер уже стоит, но мы не будем ебать вам мозг и попробуем обновить
    echo "::exec wget daedalus.ru/tester.tar -qO - | tar xf - -C /opt/tester/ ./tester ./lua"
    wget daedalus.ru/tester.tar -qO - | tar xf - -C /opt/tester/ ./tester ./lua
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
    apt-get install libc-ares2 openssl screen -y gdb python-xmpp 
elif [ -f /etc/redhat-release ];then
    rpm -Uvh http://download.fedora.redhat.com/pub/epel/5/i386/epel-release-5-3.noarch.rpm
    yum --noplugins install -y libevent libevent-devel mc zlib openssl-devel c-ares strace lua valgrind nmap  gdb python-xmpp openssl screen
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
wget daedalus.ru/tester.tar -qO - | tar xf - -C /opt/tester/


green 3* прописываем автозапуск
sed  's|exit 0||g' -i /etc/rc.local
cat >> /etc/rc.local <<!
screen -dmS tester /opt/tester/start.sh
exit 0
!

green 4* прописываем крон
cat >> /etc/crontab <<!
#clean logs
0 3 * * * root find /opt/tester/log -mtime +1  -iname "*.log" -print0 | xargs -n 20 -0 rm -f
!

green 5* hands job

echo пробуем запустить /opt/tester/tester если получилось то запускаем в бэкраунде screen -dmS tester /opt/tester/start.sh


