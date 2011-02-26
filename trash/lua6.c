#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <event.h>


// gcc -I/usr/include/lua5.1 lua5.c -o lua5 -l lua5.1 && ./lua5

lua_State *L,*thread,*thread2;
void stackDump(lua_State *L, int line) {
    int i = lua_gettop(L);
    if (i == 0) return;
    printf("----------------  Stack Dump ---------%d-------\r\n", line);
    while (i) {
        int t = lua_type(L, i);
        switch (t) {
            case LUA_TSTRING:
                printf("string %d:`%s'\n", i, lua_tostring(L, i));
                break;
            case LUA_TBOOLEAN:
                printf("bool %d: %s\n", i, lua_toboolean(L, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:
                printf("num %d: %x\n", i, lua_tonumber(L, i));
                break;
            default: printf("%d: %s %p\n", i, lua_typename(L, t), lua_topointer(L, t));
                break;
        }
        i--;
    }
    printf("--------------- Stack Dump Finished ---------------\r\n");
    fflush(stdout);
}


#define stack(L) stackDump(L,__LINE__)

  
int printtask(lua_State *L) {
    lua_pushlightuserdata(L, "task");
//    lua_getglobal( thread, "task" );
    lua_gettable(L, LUA_REGISTRYINDEX);
    char *ptr = lua_touserdata(L, -1);
//    printf("печатаем  %s %p\n",ptr,ptr);
}
              
  
int main(int argc, char *argv[])    
{   
    int i;
    char task[]="task 1";
    char task2[]="task 2";
    for(i=0;i<1000;i++) {

	thread = lua_open();   
	luaopen_base(thread);             /* opens the basic library */
	luaopen_table(thread);            /* opens the thread library */
	luaopen_string(thread);           /* opens the string lib. */
	luaopen_math(thread);             /* opens the math lib. */
	lua_pop(thread,5);
	luaL_loadstring(thread,"function main() local i=1+1 task() end");
	lua_pushcfunction(thread, printtask);    
	lua_setglobal(thread, "task"); 
	lua_pcall(thread,0,0,0);
        lua_pushlightuserdata(thread, "task");
        lua_pushlightuserdata(thread, task);
        lua_settable(thread, LUA_REGISTRYINDEX);
                        


	thread2 = lua_open();   
	luaopen_base(thread2);             /* opens the basic library */
	luaopen_table(thread2);            /* opens the thread2 library */
	luaopen_string(thread2);           /* opens the string lib. */
	luaopen_math(thread2);             /* opens the math lib. */
	lua_pop(thread2,5);
	luaL_loadstring(thread2,"function main() local i=1+1 task() end");
	lua_pushcfunction(thread2, printtask);    
	lua_setglobal(thread2, "task"); 
	lua_pcall(thread2,0,0,0);
        lua_pushlightuserdata(thread2, "task");
        lua_pushlightuserdata(thread2, task2);
        lua_settable(thread2, LUA_REGISTRYINDEX);
                        



	lua_getglobal(thread2, "main");
	lua_getglobal(thread, "main");
	lua_resume(thread, 0);
	lua_resume(thread2, 0);
	lua_close(thread);
	lua_close(thread2);

    }


    return 0;   
} 
