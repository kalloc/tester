#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <event.h>


// gcc -I/usr/include/lua5.1 lua3.c -o lua3 -l lua5.1 && ./lua3

lua_State *L;

int quit(lua_State *L) {
    printf("call quit from lua\n");
    lua_close(L);
    L=0;
}

  
int main(int argc, char *argv[])    
{   
    L = lua_open();   
    luaopen_base(L);             /* opens the basic library */
    luaopen_table(L);            /* opens the table library */
    luaopen_string(L);           /* opens the string lib. */
    luaopen_math(L);             /* opens the math lib. */
    lua_pop(L,5);
    lua_pushcfunction(L, quit);    
    lua_setglobal(L, "quit"); 
    luaL_loadstring(L,"function main() print(\".Start\") quit() print(\".nevershow\",quit)  end");
    lua_pcall(L,0,0,0);
    lua_getglobal(L, "main");
    lua_resume(L, 0);


    return 0;   
} 
