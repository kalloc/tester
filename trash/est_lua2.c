#include "include.h"
#include <openssl/evp.h>
#include <dirent.h>

#define size_t u32
#define LuaNet "api"


struct timeval tv;

static void *rootForModule = NULL;

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

int compareLua(const void *left, const void *right) {
    return strcmp(((struct struct_Lua *) left)->name, ((struct struct_Lua *) right)->name);
}

struct struct_Lua * searchLua(u_char *name, short flag) {
    struct struct_Lua *lua, *ptr;
    lua = malloc(sizeof (*lua));
    bzero(lua, sizeof (*lua));
    snprintf(lua->name, sizeof (lua->name), "%s", name);
    ptr = *(struct struct_Lua **) tsearch((void *) lua, &rootForModule, compareLua);
    if (ptr != lua) {
        free(lua);
        lua = ptr;
    } else if (flag) {
        free(lua);
        return NULL;
    }
    return lua;
};

void deleteLua(struct struct_Lua *lua) {
    tdelete((void *) lua, &rootForModule, compareLua);
    lua_close(lua->L);
    free(lua);
    lua = 0;
}

void LoadLuaFromDisk() {

    DIR *pDIR = opendir(config.lua.path);
    struct dirent *pdirent = 0;
    struct struct_Lua *lua;

    struct stat st;
    int len = 0;
    char *ptr;
    chdir(config.lua.path);
    while ((pdirent = readdir(pDIR)) > 0) {
        ptr = getLuaModuleName(pdirent->d_name);
        if (ptr > 0) {
            stat(pdirent->d_name, &st);
#ifdef DEBUG
            printf("ooh! module %s — found in %s\n", ptr, pdirent->d_name);
#endif
            lua = createLua(ptr);
            free(ptr);

            if (lua->mtime.tv_sec == 0) {
                lua->mtime = st.st_mtim;

                lua->L = lua_open();
                luaopen_base(lua->L);
                luaopen_math(lua->L);
                luaopen_string(lua->L);
                luaopen_debug(lua->L);
                luaopen_table(lua->L);
                lua_pop(lua->L, 6);
                luaL_InitEventNet(lua->L);
                luaL_loadfile(lua->L, pdirent->d_name);
                lua_pcall(lua->L, 0, 0, 0);
                lua_getglobal(lua->L, "module");
                lua_getfield(lua->L, 1, "type");
                len = 100;
                ptr = lua_tolstring(lua->L, -1, &len);
                /*
                                if (ptr[0] == 'T' or ptr[0] == 't')
                                    lua->proto = IPPROTO_TCP;
                                else
                                    lua->proto = IPPROTO_UDP;
                 */
                lua_pop(lua->L, 2);


            }
        }
    }
};

char * getLuaModuleName(const char* filename) {
    size_t len, len2;
    char *ptr, *name = 0;
    len = strlen(filename);
    if (
            len > (sizeof ("module_") + sizeof (".lua") - 1) and
            !memcmp(filename, "module_", sizeof ("module_") - 1) and
            !memcmp(filename + len - sizeof (".lua") + 1, ".lua", sizeof (".lua"))
            ) {

        lua_State *L = lua_open();
        luaopen_base(L);
        //убеждаемся что это файл  точно по формату module_*.lua ;)
        lua_pop(L, 2);
        luaL_loadfile(L, filename);
        lua_pcall(L, 0, 0, 0);
        lua_getglobal(L, "module");
        if (lua_istable(L, 1)) {
            lua_getfield(L, 1, "type");
            lua_getfield(L, 1, "name");
            if (lua_isstring(L, -1) and lua_isstring(L, -2)) {

                len2 = 100;
                lua_tolstring(L, -2, &len2);
                len = 100;
                ptr = (char *) lua_tolstring(L, -1, &len);
                if (len > 0 and len2 > 0) {
                    name = strdup(ptr);
                }
            }
            lua_pop(L, 2);
        }
        lua_pop(L, 1);
        lua_close(L);
    }
    return name;

}

void closeLuaConnection(struct Task *task) {
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    struct struct_Lua *luaTask = (struct struct_Lua *) task->lua;

#ifdef DEBUG
    printf(cGREEN"TASK"cEND" CLOSE -> id %d, Module %s for %s [%s:%d] [ms: %d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port, poll->DelayMS);
#endif

    if ((char *) poll->bev != 0) {
        bufferevent_free(poll->bev);
        poll->bev = 0;
    }

    printf("try to close task=%p\n", task);
    if (task->sub == 0) {
#ifndef TESTER
        addLuaReport(task);
#else
        incStat(task->Record.ModType, task->code);
#endif
        printf("try to close luaTask->L=%p\n", luaTask->L);
        fprintf(stderr, "Memory used = %d\n", lua_gc(luaTask->L, LUA_GCCOUNT, 0));
        luaL_unref(luaTask->M, LUA_REGISTRYINDEX, luaTask->ref);
        //lua_close(luaTask->L);
    }
    task->code = STATE_DISCONNECTED;
    shutdown(poll->fd, 1);
    close(poll->fd);

    //таск больше не нужен
    if (task->end == TRUE) {
        evtimer_del(&task->time_ev);
        deleteTask(task);
    }

};

void OnErrorLuaTask(struct bufferevent *bev, short what, void *arg) {
    struct Task *task = (struct Task *) arg;
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    int valopt = 0;
    socklen_t lon = 0;

#ifdef DEBUG
    printf(cGREEN"TASK"cEND" ERROR %s -> id %d, Module %s for %s [%s:%d] [ms: %d]\n", getActionText(what), task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port, poll->DelayMS);
#endif

    getsockopt(poll->fd, SOL_SOCKET, SO_ERROR, (void *) & valopt, &lon);
    gettimeofday(&tv, NULL);
    poll->DelayMS = (int) ((float) (tv.tv_sec + tv.tv_usec)-(float) (poll->CheckDt.tv_sec + poll->CheckDt.tv_usec)) / 1000;
    poll->CheckOk = 0;
    task->code = STATE_TIMEOUT;
    closeLuaConnection(task);

}

inline void LuaTaskResume(struct Task *task, int num) {
    struct struct_Lua *luaTask = (struct struct_Lua *) task->lua;
    if (lua_resume(luaTask->L, num) == 0) {
        lua_gc(luaTask->L, LUA_GCCOLLECT, 0);
        closeLuaConnection(task);
    }
};

void OnWriteLuaTask(struct bufferevent *bev, void *arg) {
    struct Task *task = (struct Task *) arg;
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    struct struct_Lua *luaTask = (struct struct_Lua *) task->lua;
#ifdef DEBUG
    printf(cGREEN"TASK"cEND" WRITE -> id %d, Module %s for %s [%s:%d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);
#endif
    LuaTaskResume(task, 0);
}

void timerReadLuaTask(int fd, short action, void *arg) {
    struct Task *task = (struct Task *) arg;
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    struct struct_Lua *luaTask = (struct struct_Lua *) task->lua;

    bufferevent_disable(poll->bev, EV_READ);

#ifdef DEBUG
    printf(cGREEN"TASK"cEND" timerReadLuaTask -> id %d, Module %s for %s [%s:%d] [Icmp_Code: %d, Icmp_Type: %d, ms: %d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port, poll->ProblemICMP_Code, poll->ProblemICMP_Type, poll->DelayMS);
#endif
    lua_pushnil(luaTask->L);
    LuaTaskResume(task, 1);
}

void OnReadLuaTask(struct bufferevent *bev, void *arg) {
    struct Task *task = (struct Task *) arg;
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    struct struct_Lua *luaTask = (struct struct_Lua *) task->lua;
    struct evbuffer *buffer = EVBUFFER_INPUT(bev);
    u_char *data = EVBUFFER_DATA(buffer);
    u_int len = EVBUFFER_LENGTH(buffer);

#ifdef DEBUG
    printf(cGREEN"TASK"cEND" READ -> id %d, Module %s for %s [%s:%d] [Icmp_Code: %d, Icmp_Type: %d, ms: %d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port, poll->ProblemICMP_Code, poll->ProblemICMP_Type, poll->DelayMS);
#endif
    evtimer_del(&poll->ev);

    lua_pushlstring(luaTask->L, data, len);
    evbuffer_drain(buffer, len);
    LuaTaskResume(task, 1);

}

struct Task * toLuaNet(lua_State *L, int index) {
#ifdef DEBUG
    printf(cGREEN"LUA "cEND" toLuaNet\n");
#endif
    struct Task *task = (struct Task *) lua_touserdata(L, index);
    if (task == NULL) luaL_typerror(L, index, LuaNet);
    return task;
}

struct Task ** pushLuaNet(lua_State *L, struct Task *task) {
#ifdef DEBUG
    printf(cGREEN"LUA "cEND" pushLuaNet\n");
#endif
    struct Task **ptr = (struct Task *) lua_newuserdata(L, sizeof (*task));
    *ptr = task;
    luaL_getmetatable(L, LuaNet);
    lua_setmetatable(L, -2);
    return ptr;
}

struct Task * checkLuaNet(lua_State *L, int index) {
    struct Task **ptr, *task;
    luaL_checktype(L, index, LUA_TUSERDATA);
    ptr = (struct Task **) luaL_checkudata(L, index, LuaNet);
    if (ptr == NULL) luaL_typerror(L, index, LuaNet);
    task = *ptr;
    if (!task)
        luaL_error(L, "null Image");
    return task;
}

static int LuaNetConnect(lua_State *L) {
#ifdef DEBUG
    printf(cGREEN"LUA "cEND" CONNECT\n");
#endif
    struct Task *task;
    struct sockaddr_in sa;
    struct stTCPUDPInfo *poll;


    if (lua_gettop(L) == 1) {
        lua_pushlightuserdata(L, "_task");
        lua_gettable(L, LUA_REGISTRYINDEX);
        stackDump(L, __LINE__);
        task = lua_touserdata(L, -1);
        lua_pop(L, 2);
        poll = (struct stTCPUDPInfo *) task->poll;
        gettimeofday(&((struct stTCPUDPInfo *) task->poll)->CheckDt, NULL);
    } else {

    }

    poll->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setnb(poll->fd);

    sa.sin_addr = *((struct in_addr *) & task->Record.IP);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(task->Record.Port);
    bzero(&sa.sin_zero, 8);
    connect(poll->fd, (struct sockaddr *) & sa, sizeof (struct sockaddr));
    task->code = STATE_CONNECTING;

    poll->bev = bufferevent_new(poll->fd, OnReadLuaTask, OnWriteLuaTask, OnErrorLuaTask, task);
    bufferevent_enable(poll->bev, EV_WRITE);
    bufferevent_settimeout(poll->bev, task->Record.CheckPeriod / 2, task->Record.CheckPeriod / 2);
    pushLuaNet(L, task);
    lua_yield(L, 1);
    return 1;
}

static int LuaNet_gc(lua_State *L) {
    struct struct_Lua *ptr = toLuaNet(L, 1);
    printf("goodbye LuaNet(%p)\n", lua_touserdata(L, 1));
    return 0;
}

static int LuaNet_tostring(lua_State *L) {
#ifdef DEBUG
    printf(cGREEN"LUA "cEND" LuaNet_tostring\n");
#endif
    lua_pushfstring(L, "LuaNet: %p", lua_touserdata(L, 1));
    return 1;
}

static const luaL_reg EventNet_meta[] = {
    {"__gc", LuaNet_gc},
    {"__tostring", LuaNet_tostring},
    {0, 0}
};

static int LuaNetHash(lua_State * L) {
    struct Task *task = checkLuaNet(L, 1);
    EVP_MD_CTX mdctx;
    const EVP_MD *md;
    unsigned char md_value[EVP_MAX_MD_SIZE];
    char *algorithm = NULL;
    char *digest = NULL;
    char *cur = NULL;
    unsigned int md_len = 0;
    unsigned int arguments = 0;
    unsigned int i = 0;
    size_t msg_len = 0;

    // Get the algorithm name from the closure
    algorithm = (char *) lua_tostring(L, 1);

    // Get the number of stack arguments
    arguments = lua_gettop(L);

    // Get the digest
    OpenSSL_add_all_digests();
    md = EVP_get_digestbyname(algorithm);
    if (!md) {
        lua_pushfstring(L, "No such hash algorithm: %s", algorithm);
        return lua_error(L);
    }

    // Initialise the hash context
    EVP_MD_CTX_init(&mdctx);
    EVP_DigestInit_ex(&mdctx, md, NULL);

    // Add the arguments to the hash.
    for (i = 2; i <= arguments; i++) {
        cur = (char *) lua_tolstring(L, i, &msg_len);
        EVP_DigestUpdate(&mdctx, cur, msg_len);
    }

    // Finalise the hash
    EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
    EVP_MD_CTX_cleanup(&mdctx);

    // Convert the hash to a string
    msg_len = 1 + 2 * md_len;
    cur = digest = (char*) malloc(msg_len);
    for (i = 0; i < md_len; i++) {
        snprintf(cur, 3, "%02x", md_value[i]);
        cur = cur + 2;
    }
    cur[0] = '\0';

    // Push the result onto the stack
    lua_pushstring(L, digest);
    free(digest);

    // Return the number of return values
    return (1);
}

static int LuaNetRead(lua_State * L) {
    struct Task *task = checkLuaNet(L, 1);
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    task->code = STATE_READ;


#ifdef DEBUG
    printf(cGREEN"LUA "cEND" READ\n");
#endif
    printf("%d %p %p %p\n", task->LObjId, poll, task, L);

    bufferevent_enable(poll->bev, EV_READ);

    timerclear(&tv);
    tv.tv_usec = 5000;
    evtimer_set(&poll->ev, timerReadLuaTask, task);
    evtimer_add(&poll->ev, &tv);
    return lua_yield(L, 1);
}

static int LuaNetWrite(lua_State * L) {
    int len = 4096;


    struct Task *task = checkLuaNet(L, 1);
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;

    char *ptr = lua_tolstring(L, 2, &len);
    task->code = STATE_WRITE;
#ifdef DEBUG
    printf(cGREEN"LUA "cEND" WRITE %d bytes\n", len);
#endif
    printf("%d %p %p %p\n", task->LObjId, poll, task, L);

    bufferevent_write(poll->bev, ptr, len);
    return lua_yield(L, 0);
}

static int LuaNetError(lua_State * L) {
#ifdef DEBUG
    printf(cGREEN"LUA "cEND" ERROR\n");
#endif
    struct Task *task = checkLuaNet(L, 1);
    task->code = STATE_ERROR;
    return 0;
}

static int LuaNetDone(lua_State * L) {
#ifdef DEBUG
    printf(cGREEN"LUA "cEND" DONE\n");
#endif
    struct Task *task = checkLuaNet(L, 1);
    task->code = STATE_DONE;
    return 0;
}

int luaL_InitEventNet(lua_State * L) {
    static const luaL_Reg RegisterEventNet[] = {
        {"connect", LuaNetConnect},
        {"read", LuaNetRead},
        {"write", LuaNetWrite},
        {"done", LuaNetDone},
        {"error", LuaNetError},
        {"hash", LuaNetHash},
        {NULL, NULL}
    };

    luaL_openlib(L, LuaNet, RegisterEventNet, 0);
    luaL_newmetatable(L, LuaNet);
    luaL_openlib(L, 0, EventNet_meta, 0); /* fill metatable */
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3); /* dup methods table*/
    lua_rawset(L, -3); /* metatable.__index = methods */
    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, -3); /* dup methods table*/
    lua_rawset(L, -3);
    lua_pop(L, 1);

    return 1; /* return methods on the stack */
}

void timerLuaTask(int fd, short action, void *arg) {
    struct Task *task = (struct Task *) arg;
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    struct struct_Lua *luaModule, *luaTask;

    luaTask = (struct struct_Lua *) task->lua;
    luaModule = getLua(luaTask->name);
#ifdef DEBUG
    printf(cGREEN"LUA "cEND" runLuaTask - %s\n", luaModule->name);
#endif



    if (luaModule == NULL) {
        incStat(task->Record.ModType, STATE_ERROR);
        task->end == TRUE;
        return;
    }

    if (task->end == TRUE) {
        closeLuaConnection(task);
        return;
    }

    luaTask->M = luaModule->L;
    luaTask->L = lua_newthread(luaModule->L);

    lua_pushvalue(luaModule->L, -1);
    luaTask->ref = luaL_ref(luaModule->L, LUA_REGISTRYINDEX);

    lua_pushthread(luaTask->L, "_task");
    lua_pushlightuserdata(luaTask->L, task);
    lua_settable(luaTask->L, LUA_REGISTRYINDEX);


    stackDump(luaTask->L, __LINE__);
    lua_newtable(luaTask->L);
    lua_pushstring(luaTask->L, task->Record.HostName);
    lua_setfield(luaTask->L, 1, "host");
    lua_pushinteger(luaTask->L, task->Record.Port);
    lua_setfield(luaTask->L, 1, "port");
    lua_setfield(luaTask->L, LUA_GLOBALSINDEX, "argv");



    /*

        luaL_getmetatable(luaTask->L, LuaNet);
        lua_setmetatable(luaTask->L, -1);
     */


    lua_getglobal(luaTask->L, "main");
    stackDump(luaTask->L, __LINE__);


    timerclear(&tv);
    if (task->Record.CheckPeriod) {
        tv.tv_sec = task->Record.CheckPeriod;
    } else {
        task->end = TRUE;
        tv.tv_sec = 10;
    }
    evtimer_add(&task->time_ev, &tv);

    LuaTaskResume(task, 0);

}

void initLUATester() {
    LoadLuaFromDisk();
}

// gcc -I /usr/include/lua5.1/  curve25519-donna/curve25519-donna.c  -levent  -llua5.1 -lssl yxml.c tools.c test_lua.c -o test_lua && ./test_lua

/*
int main(int argc, char *argv[]) {
    struct struct_Lua *luaTask;
    printf("1 runLuaTask(\"test\")\n");
    luaTask = runLuaTask("test");
    if (luaTask == NULL) {
        printf("пиздец\n");
        return -1;
    }
    printf("2. resumeLuaTask(luaTask, NULL,0)\n");
    resumeLuaTask(luaTask, NULL, 0);

    printf("3. resumeLuaTask(luaTask, \"blabla\", 7)\n");
    resumeLuaTask(luaTask, "blabla", 7);

    return 0;
}
 */

