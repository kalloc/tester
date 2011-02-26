#include <stdio.h>

void calc(
    int t,
    int s,
    int ps,
    int pcd,
    int cd
) {












    int period=60;
    int timer;
    timer  = (cd-ps)%60;
    printf("t: %02d, s: %02d, ps: %02d,pcd: %02d, cd: %02d, timer: %02d\n",t,s,ps,pcd,cd,timer);
    
}
int main() {

    calc(1254880734,45,55,1254880675,1254880785);
    calc(1254880794,35,45,1254880785,1254880835);

    calc(1254880867,52,35,1254880835,1254880912);

    calc(1254880922,28,52,1254880912,1254880948);
    calc(1254880982,33,28,1254880948,1254881013);
    calc(1254881042,9,33,1254881013,1254881049);
    calc(1254881102,45,9,1254881049,1254881145);


}









/*

[01:58:54 1254880734] [DEBUG] tester.c:addLuaTask() Lobj 1056 NewRemainder = 45, OldRemainder = 55,  1254880675, NextCheckDt = 1254880785
[01:59:54 1254880794] [DEBUG] tester.c:addLuaTask() Lobj 1056 NewRemainder = 35, OldRemainder = 45,  1254880785, NextCheckDt = 1254880835

[02:01:07 1254880867] [DEBUG] tester.c:addLuaTask() Lobj 1056 NewRemainder = 52, OldRemainder = 35,  1254880835, NextCheckDt = 1254880912

[02:02:02 1254880922] [DEBUG] tester.c:addLuaTask() Lobj 1056 NewRemainder = 28, OldRemainder = 52,  1254880912, NextCheckDt = 1254880948
[02:03:02 1254880982] [DEBUG] tester.c:addLuaTask() Lobj 1056 NewRemainder = 33, OldRemainder = 28,  1254880948, NextCheckDt = 1254881013
[02:04:02 1254881042] [DEBUG] tester.c:addLuaTask() Lobj 1056 NewRemainder = 9, OldRemainder = 33,  1254881013, NextCheckDt = 1254881049
[02:05:02 1254881102] [DEBUG] tester.c:addLuaTask() Lobj 1056 NewRemainder = 45, OldRemainder = 9,  1254881049, NextCheckDt = 1254881145
*/
