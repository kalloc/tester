#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <event.h>

#define size_t u32
#define len 100000


struct Bla {
    char bla[50];
    lua_State *thread;
    int table;

} bla[len];


struct timeval tv;

struct Bla * toBla(lua_State *L, int index) {
    struct Bla *bla = (struct Bla *) lua_touserdata(L, index);
    if (bla == NULL) luaL_typerror(L, index, "Bla");
    return bla;
}


struct Bla ** pushBla(lua_State *L, struct Bla *bla) {
    struct Bla  **ptr = (struct Bla *) lua_newuserdata(L, sizeof (*bla));
    *ptr = bla;
    luaL_getmetatable(L, "Bla");
    lua_setmetatable(L, 2);
    return ptr;
}

struct Bla * checkBla(lua_State *L, int index) {
    struct Bla **ptr, *bla;
    luaL_checktype(L, index, LUA_TUSERDATA);
    ptr = (struct Bla **) luaL_checkudata(L, index, "Bla");
    if (ptr == NULL) luaL_typerror(L, index, "Bla");
    bla = *ptr;
    return bla;
}



// gcc -I/usr/include/lua5.1 lua7.c -o lua7 -l lua5.1 && ./lua7



static int Bla_gc(lua_State *L) {
    struct struct_Lua *ptr = toBla(L, 1);
//    printf("goodbye Bla(%p)\n", lua_touserdata(L, 1));
    return 0;
}
            
static int Bla_tostring(lua_State *L) {
    lua_pushfstring(L, "%p",((struct Bla *)checkBla(L,1))->thread);
    return 1;
}


static int BlaNew(lua_State *L) {
    pushBla(L, bla);
    return 1;
}


static int BlaBla(lua_State * L) {
    struct Bla *bla = checkBla(L, 1);
    lua_pushstring(L,bla->bla);
    return 1;
}
static int BlaSleep(lua_State * L) {
    return lua_yield(L,0);
}


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
    static const luaL_reg EventNet_meta[] = {
	{"__gc", Bla_gc},
        {"__tostring", Bla_tostring},
        {0, 0}
    };
            
    static const luaL_Reg RegisterEventNet[] = {
        {"new", BlaNew},
        {"sleep", BlaSleep},
        {"bla", BlaBla},
        {NULL, NULL}
    };

    luaL_openlib(L, "Bla", RegisterEventNet, 0);
    luaL_newmetatable(L, "Bla");
    luaL_openlib(L, 0, EventNet_meta, 0);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
            
    lua_pop(L, 2);

    luaL_loadstring(L,"function main() local bla=Bla:new() bla:sleep() bla:bla() end");



    int count=0;
    lua_pcall(L,0,0,0);
    printf("Memory used = %d\n",lua_gc(L, LUA_GCCOUNT, 0));
    for(i=0;i<1;i++) {
	for(ii=0;ii<=len;ii++) {
	    count++;
	    snprintf(bla[ii].bla,50,"bla%d",ii);
	    bla[ii].thread = lua_newthread(L);
	    bla[ii].table  = luaL_ref (L,LUA_REGISTRYINDEX);

	    lua_pushthread(bla[ii].thread);
            lua_pushlightuserdata(bla[ii].thread,(void*) &bla[ii]);
            lua_settable(bla[ii].thread, LUA_REGISTRYINDEX);
	    lua_getglobal(bla[ii].thread, "main");
	    lua_resume(bla[ii].thread, 0);
//	    printf("init %s %p\n",bla[ii].bla,bla[ii].thread);
	    
	}
	for(ii=0;ii<=len;ii++) {
	    lua_resume(bla[ii].thread, 0);
//	    printf("in c bla[%d] = %p and bla string is    %s\n\n",ii,bla[ii].thread,bla[ii].bla);
	    lua_pushthread(bla[ii].thread);
	    lua_pushnil(bla[ii].thread);
	    lua_settable(bla[ii].thread, LUA_REGISTRYINDEX);
	    luaL_unref(L, LUA_REGISTRYINDEX, bla[ii].table );
	}
    }
    printf("Memory used = %d, cycle %d\n",lua_gc(L, LUA_GCCOUNT, 0),count);
    lua_gc(L, LUA_GCCOLLECT, 0);
    printf("Memory used = %d, cycle %d\n",lua_gc(L, LUA_GCCOUNT, 0),count);
    lua_close(L);


    return 0;   
} 