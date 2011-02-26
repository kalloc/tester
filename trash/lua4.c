#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <event.h>


// gcc -I/usr/include/lua5.1 lua4.c -o lua4 -l lua5.1 && ./lua4

lua_State *L,*thread,*thread2;
  
int main(int argc, char *argv[])    
{   
    int i;
    L = lua_open();   
    int table,table2;
    luaopen_base(L);             /* opens the basic library */
    luaopen_table(L);            /* opens the table library */
    luaopen_string(L);           /* opens the string lib. */
    luaopen_math(L);             /* opens the math lib. */
    lua_pop(L,5);
    luaL_loadstring(L,"function main() local i=1+1 print(i+666) end");
    lua_pcall(L,0,0,0);
    
    for(i=0;i<100000;i++) {

	thread=lua_newthread(L);
	thread2=lua_newthread(L);
        table = luaL_ref (L,LUA_REGISTRYINDEX);
        table2 = luaL_ref (L,LUA_REGISTRYINDEX);
	lua_getglobal(thread, "main");
	lua_resume(thread, 0);
	lua_getglobal(thread2, "main");
	lua_resume(thread2, 0);
        luaL_unref(L, LUA_REGISTRYINDEX, table );
        luaL_unref(L, LUA_REGISTRYINDEX, table2 );
    }
    lua_close(L);


    return 0;   
} 
