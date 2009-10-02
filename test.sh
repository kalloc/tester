#!/bin/bash
clear
function run() {
printf "\e[1;32m$1\e[0m \e[1;33m $2 \e[0m \e[1;34m try $4 task's \e[0m"
printf "\t*compile...."
gcc -O0 -g  -DTEST_$1  $3 -DCHECKPERIOD=0  -DCOUNT=$4  -I /usr/include/lua5.1/  curve25519-donna/curve25519-donna.c  -levent -lm -lssl yxml.c statistics.c tester.c tools.c test_*.c ./liblua.a -o test_lua 
echo "run"
time ./test_lua || exit
}



printf "\e[1;31m
    в режиме эмуляции ошибки (ERROR) в скрипте возвращается ошибка в конце выполнения
    		      таймаута (TIMEOUT). любой порт проверки заменяется на 12341
\n\e[0m"



printf "\e[1;35m\t\t\t _.::[ FTP ]::._\e[0m\n"
run "FTP" "Normal 213.248.62.7" "" 10
run "FTP" "Emu Mode - Error" "-DREPORT_ERROR" 10
run "FTP" "Emu Mode - Timeout" "-DREPORT_TIMEOUT" 10
echo "-------------------------------------------------------------------"

printf "\e[1;35m\t\t\t _.::[ POP ]::._\e[0m\n"
run "POP" "Normal 213.248.62.7" "" 10
run "POP" "Emu Mode - Error" "-DREPORT_ERROR" 10
run "POP" "Emu Mode - Timeout" "-DREPORT_TIMEOUT" 10
echo "-------------------------------------------------------------------"



printf "\e[1;35m\t\t\t _.::[ HTTP ]::._\e[0m\n"
run "HTTP" "Normal localhost" "" 1000
run "HTTP" "Emu Mode - Error" "-DREPORT_ERROR" 1000
run "HTTP" "Emu Mode - Timeout" "-DREPORT_TIMEOUT" 1000
echo "-------------------------------------------------------------------"

printf "\e[1;35m\t\t\t _.::[ TCP ]::._\e[0m\n"
run "TCP" "Normal Localhost " "" 5432
echo "-------------------------------------------------------------------"


printf "\e[1;35m\t\t\t _.::[ PING ]::._\e[0m\n"
run "PING" "Normal Localhost" "" 5432
echo "-------------------------------------------------------------------"


printf "\e[1;35m\t\t\t _.::[ ALL ]::._\e[0m\n"
run "ALL" "TEST ALL / Normal" "" 1000
run "ALL" "TEST ALL / Emu Mode - Error" "-DREPORT_ERROR" 1000
run "ALL" "TEST ALL / Emu Mode - Timeout" "-DREPORT_TIMEOUT" 1000
echo "-------------------------------------------------------------------"
