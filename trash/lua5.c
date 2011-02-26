#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


// gcc -I/usr/include/lua5.1 lua5.c -o lua5 -l lua5.1 && ./lua5

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
    lua_pushthread(L);
    lua_gettable(L, LUA_REGISTRYINDEX);
    char *ptr = lua_touserdata(L, -1);        
//    stack(L);
//    printf("печатаем  %s %p %p, Memory used = %d\n",ptr,ptr,L,lua_gc(L, LUA_GCCOUNT, 0));
    lua_yield(L,0);
}
              
#define len 40000
  
struct Bla {

    char task[50];
    lua_State *thread;
    int table;

} bla[len];
int main(int argc, char *argv[])    
{   
    int i,ii;
    lua_State *L;
    L = lua_open();   
    luaopen_base(L);             /* opens the basic library */
    luaopen_table(L);            /* opens the table library */
    luaopen_string(L);           /* opens the string lib. */
    luaopen_math(L);             /* opens the math lib. */
    lua_pop(L,5);
    luaL_loadstring(L,"function main() local i=1+1 task()  end");

    lua_pushcfunction(L, printtask);    
    lua_setglobal(L, "task"); 
    int count=0;
    lua_pcall(L,0,0,0);
    printf("Memory used = %d\n",lua_gc(L, LUA_GCCOUNT, 0));
    for(ii=0;ii<=len;ii++) {
	    count++;
	    snprintf(bla[ii].task,50,"task%d",ii);
	    bla[ii].thread = lua_newthread(L);
	    bla[ii].table  = luaL_ref (L,LUA_REGISTRYINDEX);

	    lua_pushthread(bla[ii].thread);
            lua_pushlightuserdata(bla[ii].thread, bla[ii].task);
            lua_settable(bla[ii].thread, LUA_REGISTRYINDEX);
	    //printf("init %s %p %p\n",bla[ii].task,bla[ii].task,bla[ii].thread);
	    //stack(bla[ii].thread);
	    lua_getglobal(bla[ii].thread, "main");
	    lua_resume(bla[ii].thread, 0);

    }
    for(ii=0;ii<=len;ii++) {
	    lua_resume(bla[ii].thread, 0);
        
	    lua_pushthread(bla[ii].thread);
	    lua_pushnil(bla[ii].thread);
	    lua_settable(bla[ii].thread, LUA_REGISTRYINDEX);
	    

//	    printf("печатаем  %s\n",lua_touserdata(bla[ii].thread, -1));
//	    lua_remove(bla[ii].thread,-1);
//	    stack(bla[ii].thread);
//	    printf("печатаем  %s\n",lua_touserdata(bla[ii].thread, -1));
	    luaL_unref(L, LUA_REGISTRYINDEX, bla[ii].table );
    }
    printf("Memory used = %d, cycle %d\n",lua_gc(L, LUA_GCCOUNT, 0),count);
    lua_gc(L, LUA_GCCOLLECT, 0);
    printf("Memory used = %d, cycle %d\n",lua_gc(L, LUA_GCCOUNT, 0),count);
    lua_close(L);


    return 0;   
} 
