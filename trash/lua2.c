#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <event.h>


// gcc -I/usr/include/lua5.1 lua2.c -o lua2 -levent -l lua5.1 && ./lua2 
lua_State *L;

struct event time_ev,time_ev2,time_ev3;
inline void luaResume(lua_State *L,int num) {
    int ret=lua_resume(L, num);
    if(!ret) {
	printf("закрываем L:%p\n",L);
//	lua_close(L);
//	L=0;
//	printf("закрываем L:%p\n",L);
    }

}
void alarm(int fd, short action, void *arg) {
    lua_State *L=(lua_State *)arg;
    printf("resume from wait to lua\n");
    luaResume(L,0);
}



int wait(lua_State *L) {

    printf("wait lua_status(L)=%d\n",lua_status(L));
    struct timeval tv;
    timerclear(&tv);
    tv.tv_sec=1;
    printf("call wait from lua\n");
    evtimer_set(&time_ev3, alarm, (void *)L);
    evtimer_add(&time_ev3, &tv);
    return lua_yield(L,0);
}


int quit(lua_State *L) {
    printf("call quit from lua\n");
    lua_close(L);
    L=0;
}

void timerLuaTask(int fd, short action, void *arg) {
    struct timeval tv;
    lua_State *thread,*L=(lua_State *)arg;

    timerclear(&tv);
    tv.tv_sec=10;
    printf("timer,run main task\n");
    evtimer_set(&time_ev, timerLuaTask,0);
    evtimer_add(&time_ev, &tv);

    thread=lua_newthread(L);
    luaL_loadstring(thread,"function main() print(\".Start\") wait() wait() print(\".End\",quit) end");
    lua_pcall(thread,0,0,0);
    lua_getglobal(thread, "main");
    printf("timer do resume lua_status(L)=%d\n",lua_status(thread));

    luaResume(thread,0);
    printf("timer posle resume lua_status(L)=%d\n",lua_status(thread));
}



void timer(int fd, short action, void *arg) {
}

  
int main(int argc, char *argv[])    
{   
    lua_State *L = lua_open();   
    luaopen_base(L);             /* opens the basic library */
    luaopen_table(L);            /* opens the table library */
    luaopen_string(L);           /* opens the string lib. */
    luaopen_math(L);             /* opens the math lib. */
    lua_pop(L,5);
    lua_pushcfunction(L, wait);    
    lua_setglobal(L, "wait"); 
    lua_pushcfunction(L, quit);    
    lua_setglobal(L, "quit"); 

    struct timeval tv;
    event_init();
    timerclear(&tv);
    evtimer_set(&time_ev, timerLuaTask,L);
    evtimer_add(&time_ev, &tv);
    event_dispatch();
    return 0;   
} 
