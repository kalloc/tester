#include "include.h"
#include <dirent.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <iconv.h>
//static struct event_base *base = 0L;



#define size_t u32
#define LuaNet "api"

unsigned cycles = 0;
int getLuaModuleId(const char*);
struct Task * checkLuaNet(lua_State *, int);
inline int LuaTaskResume(struct Task *task, int num);
void RunLuaTask(struct Task *task);
static int LuaNetClose(lua_State * L);
static int LuaNetBase64Encode(lua_State * L);
static int LuaNetBase64Decode(lua_State * L);
static int LuaNetError(lua_State * L);
static int LuaNetConnect(lua_State * L);
static int LuaNetDone(lua_State * L);
static int LuaNetRead(lua_State * L);
static int LuaNetSleep(lua_State * L);
static int LuaNetWrite(lua_State * L);
static int LuaNetHash(lua_State * L);
static int LuaNetHMAC(lua_State * L);
static int LuaNetIconv(lua_State * L);
inline int LuaTaskResume(struct Task *, int);
static int LuaNet_gc(lua_State *L);
static int LuaNet_tostring(lua_State *L);
void timerLuaNetRead(int fd, short action, void *arg);
struct timeval tv;

static void *rootForModule = NULL;

int compareLua(const void *left, const void *right) {
    if (((struct struct_LuaModule *) left)->ModType > ((struct struct_LuaModule *) right)->ModType) {
        return -1;
    }
    if (((struct struct_LuaModule *) left)->ModType < ((struct struct_LuaModule *) right)->ModType) {
        return 1;
    }
    return 0;
}

void deleteLua(struct struct_LuaModule *lua) {
    tdelete((void *) lua, &rootForModule, compareLua);
    if (lua->state) lua_close(lua->state);
    free(lua);
    lua = 0;
}

struct struct_LuaModule * searchLua(int id, short flag) {
    struct struct_LuaModule *lua, *ptr;
    lua = (struct struct_LuaModule *) getNulledMemory(sizeof (*lua));
    lua->ModType = id;
    ptr = *(struct struct_LuaModule **) tsearch((void *) lua, &rootForModule, compareLua);
    if (ptr != lua) {
        free(lua);
        lua = ptr;
    } else if (flag) {
        deleteLua(lua);
        return NULL;
    }
    return lua;
};

static void timerMain(int fd, short action, void *arg) {
    struct struct_LuaModule *lua = (struct struct_LuaModule *) arg;
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    evtimer_add(&lua->timer, &tv);
}

void initLuaThread(void *arg) {
    struct struct_LuaModule *lua = (struct struct_LuaModule *) arg;
    lua->base = event_base_new();
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    event_assign(&lua->timer, lua->base, -1, 0, timerMain, lua);
    evtimer_add(&lua->timer, &tv);
    event_base_dispatch(lua->base);
}

int luaL_InitEventNet(lua_State * L) {
    static const luaL_Reg RegisterEventNet[] = {
        {"connect", LuaNetConnect},
        {"read", LuaNetRead},
        {"close", LuaNetClose},
        {"write", LuaNetWrite},
        {"done", LuaNetDone},
        {"sleep", LuaNetSleep},
        {"error", LuaNetError},
        {NULL, NULL}
    };

    static const luaL_reg EventNet_meta[] = {
        {"__tostring", LuaNet_tostring},
        //        {"__gc", LuaNet_gc},
        {NULL, NULL}
    };

    lua_getglobal(L, "string");
    lua_pushliteral(L, "base64encode");
    lua_pushcfunction(L, LuaNetBase64Encode);
    lua_settable(L, 1);
    lua_pushliteral(L, "base64decode");
    lua_pushcfunction(L, LuaNetBase64Decode);
    lua_settable(L, 1);

    lua_pushliteral(L, "hash");
    lua_pushcfunction(L, LuaNetHash);
    lua_settable(L, 1);


    lua_pushliteral(L, "iconv");
    lua_pushcfunction(L, LuaNetIconv);
    lua_settable(L, 1);

    lua_pushliteral(L, "hmac");
    lua_pushcfunction(L, LuaNetHMAC);
    lua_settable(L, 1);


    luaL_openlib(L, LuaNet, RegisterEventNet, 0);
    luaL_newmetatable(L, LuaNet);
    luaL_openlib(L, 0, EventNet_meta, 0);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);

    lua_pop(L, 2);

    return 1; /* return methods on the stack */
}

void LoadLuaFromDisk() {

    char filenameWithPath[4096];
    DIR *pDIR = opendir(config.lua.path);
    struct dirent *pdirent = 0;
    struct struct_LuaModule *lua;

    struct stat st;
    int ModType;
    while ((pdirent = readdir(pDIR)) > 0) {
        ModType = getLuaModuleId(pdirent->d_name);
        if (ModType > 0) {
            snprintf(filenameWithPath, 4096, "%s/%s", config.lua.path, pdirent->d_name);
            stat(pdirent->d_name, &st);
            info("module %s — found in %s", getModuleText(ModType), pdirent->d_name);
            lua = createLua(ModType);


            if (lua->mtime.tv_sec == 0) {
                lua->mtime = st.st_mtim;

                lua->state = lua_open();
                luaopen_base(lua->state);
                luaopen_math(lua->state);
                luaopen_string(lua->state);
                luaopen_debug(lua->state);
                luaopen_table(lua->state);
                lua_pop(lua->state, 6);
                luaL_InitEventNet(lua->state);
                luaL_loadfile(lua->state, filenameWithPath);
                lua_pcall(lua->state, 0, 0, 0);
                //lua_getglobal(lua->L, "module");
                //lua_getfield(lua->L, 1, "type");
                //len = 100;
                /*
                 ptr = (char *) lua_tolstring(lua->L, -1, &len);
                
                                if (ptr[0] == 'T' or ptr[0] == 't')
                                    lua->proto = IPPROTO_TCP;
                                else
                                    lua->proto = IPPROTO_UDP;
                 */
                //lua_pop(lua->L, 2);
                pthread_create(&lua->threads, NULL, (void*) initLuaThread, lua);


            }
        }
    }
    closedir(pDIR);
};

int getLuaModuleId(const char* filename) {
    size_t len;
    char filenameWithPath[4096];
    snprintf(filenameWithPath, 4096, "%s/%s", config.lua.path, filename);

    int ModType = 0;
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
        luaL_loadfile(L, filenameWithPath);
        lua_pcall(L, 0, 0, 0);
        lua_getglobal(L, "module");
        if (lua_istable(L, 1)) {
            lua_getfield(L, 1, "id");
            if (lua_isnumber(L, -1)) {
                ModType = lua_tonumber(L, -1);
            }
            lua_pop(L, 2);
        }
        lua_pop(L, 1);
        lua_close(L);
    }
    return ModType;

}

void closeLuaConnection(struct Task *task) {
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    struct struct_LuaTask *luaTask = (struct struct_LuaTask *) task->ptr;
    gettimeofday(&tv, NULL);
    poll->DelayMS = timeDiffMS(tv, poll->CheckDt);

    debug("id %d, Module %s for %s [%s:%d] [ms: %d] %s",
            task->LObjId,
            getModuleText(task->Record.ModType),
            task->Record.HostName,
            ipString(task->Record.IP),
            task->Record.Port,
            poll->DelayMS,
            task->isSub ? "is sub" : "");

    /*
        } else if (task->code != STATE_DISCONNECTED) {
            lua_remove(luaTask->L, 1);
            task->lua = 0;
     */

    if (task->code == STATE_CONNECTING || task->code == STATE_CONNECTED) {
        task->code = STATE_TIMEOUT;
    } else if (task->code != STATE_DONE and task->code != STATE_TIMEOUT) {
        task->code = STATE_ERROR;
    }
#ifndef TESTER
    if (task->callback) {
        task->callback(task);
    }
#else
    incStat(task->Record.ModType, task->code);
#endif


    if (task->isRead) {
        task->isRead = 0;
        evtimer_del(&task->read);
    }



    task->code = STATE_DISCONNECTED;

    if ((char *) poll->bev) {
        bufferevent_free(poll->bev);
        poll->bev = 0;
    }


    if (luaTask) {
        if (luaTask->issleep) {
            luaTask->issleep = 0;
            evtimer_del(&luaTask->sleep);
        }
        if (luaTask->state) {
            lua_pushthread(luaTask->state);
            lua_pushnil(luaTask->state);
            lua_settable(luaTask->state, LUA_REGISTRYINDEX);
            luaL_unref(luaTask->luaModule->state, LUA_REGISTRYINDEX, luaTask->ref);

            LIST_FOREACH(LuaNetListEntryPtr, &luaTask->LuaNetListHead, entries) {
                if (LuaNetListEntryPtr->task) {
                    if (LuaNetListEntryPtr->task->isRead) {
                        LuaNetListEntryPtr->task->isRead = 0;
                        evtimer_del(&LuaNetListEntryPtr->task->read);
                    }

                    if ((char *) ((struct stTCPUDPInfo *) (LuaNetListEntryPtr->task->poll))->bev) {

                        bufferevent_free(((struct stTCPUDPInfo *) (LuaNetListEntryPtr->task->poll))->bev);
                        ((struct stTCPUDPInfo *) (LuaNetListEntryPtr->task->poll))->bev = 0;
                    }

                    deleteTask(LuaNetListEntryPtr->task);
                }
                LIST_REMOVE(LuaNetListEntryPtr, entries);
                free(LuaNetListEntryPtr);
            }
        }
    }

    if (task->isEnd) {
        //таск больше не нужен
        evtimer_del(&task->time_ev);
        //printf("deleteTask(task) = %p\n", task);
        deleteTask(task);
        task = 0;

    }


};

int LuaTaskResume(struct Task *task, int num) {
    struct struct_LuaTask *luaTask = (struct struct_LuaTask *) task->ptr;
    int ret = lua_status(luaTask->state);
    if (ret != 1) return ret;
    ret = lua_resume(luaTask->state, num);

    if (ret == 0) {
        if (task and task->isSub) {
            task = luaTask->ptask;
        }
        closeLuaConnection(task);
    } else if (ret > 1) {
        stack(luaTask->state);
        task->code = STATE_ERROR;
    }
    return ret;
};

void OnErrorLuaTask(struct bufferevent *bev, short what, void *arg) {
    struct Task *task = (struct Task *) arg;
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    if (what & BEV_EVENT_CONNECTED or what & BEV_EVENT_EOF) {

        if (task->code == STATE_CONNECTING) {
            task->code = STATE_CONNECTED;
            RunLuaTask(task);
        }
        return;
    }

    debug("%s -> id %d, timeout is %d, State %s, Module %s for %s [%s:%d] %s", getActionText(what), task->LObjId, task->Record.TimeOut, getStatusText(task->code), getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port, task->isSub ? "is sub" : "");

    int valopt = 0;
    socklen_t lon = 0;


    getsockopt(bufferevent_getfd(poll->bev), SOL_SOCKET, SO_ERROR, (void *) & valopt, &lon);
    poll->CheckOk = 0;
    task->code = STATE_TIMEOUT;
    closeLuaConnection(task);


}

void OnWriteLuaTask(struct bufferevent *bev, void *arg) {
    struct Task *task = (struct Task *) arg;
    debug("%s-> id %d, Module %s for %s [%s:%d] %s", getStatusText(task->code), task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port, task->isSub ? "is sub" : "");
    if (task->code == STATE_WRITE) {
        task->code = STATE_CONNECTED;
        LuaTaskResume(task, 0);

    }

}

void OnReadLuaTask(struct bufferevent *bev, void *arg) {
    struct Task *task = (struct Task *) arg;
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    struct struct_LuaTask *luaTask = (struct struct_LuaTask *) task->ptr;
    struct evbuffer *buffer = EVBUFFER_INPUT(bev);
    u_char *data = EVBUFFER_DATA(buffer);
    u_int len = EVBUFFER_LENGTH(buffer);

    debug("%d byte -> id %d, Module %s for %s [%s:%d] %s", len, task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port, task->isSub ? "is sub" : "");
    if (len > 0 and len != task->readedSize and config.maxInput > len) {
        task->code = STATE_READ;

        task->readedSize = len;

        timerclear(&tv);
#define MIN_TTL 100
        if (task->Record.TimeOut > MIN_TTL) {
            tv.tv_usec = MIN_TTL * 1000;
        } else {
            tv.tv_usec = task->Record.TimeOut * 1000;
        }
        bufferevent_enable(poll->bev, EV_READ);
        evtimer_add(&task->read, &tv);
    } else {
        evtimer_del(&task->read);
        task->isRead = 0;

        if (task->readedSize == 0) {
            lua_pushnil(luaTask->state);
        } else {
            lua_pushlstring(luaTask->state, (char*) data, len);
            task->readedSize = 0;
        }

        evbuffer_drain(buffer, len);
        LuaTaskResume(task, 1);
    }
}

void OnErrorLuaSubTask(struct bufferevent *bev, short what, void *arg) {
    struct Task *task = (struct Task *) arg;
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    struct struct_LuaTask *luaTask = (struct struct_LuaTask *) task->ptr;


    debug("%s -> id %d,State %s, Module %s for %s [%s:%d] [ms: %d]",
            getActionText(what),
            task->LObjId,
            getStatusText(task->code),
            getModuleText(task->Record.ModType),
            task->Record.HostName, ipString(task->Record.IP), task->Record.Port, poll->DelayMS);

    if ((what & BEV_EVENT_CONNECTED)) {

        if (task->code == STATE_CONNECTING) {
            task->code = STATE_CONNECTED;
            LuaTaskResume(task, 1);
        }
        return;
    }
    struct evbuffer *buffer = EVBUFFER_INPUT(poll->bev);
    u_char *data = EVBUFFER_DATA(buffer);
    u_int len = EVBUFFER_LENGTH(buffer);

    if (len > 0) {
        lua_pushlstring(luaTask->state, (char *) data, len);
        task->readedSize = 0;
        LuaTaskResume(task, 1);
    }
}

void OnWriteLuaSubTask(struct bufferevent *bev, void *arg) {
    struct Task *task = (struct Task *) arg;
    debug("%s -> id %d, Module %s for %s [%s:%d]", getStatusText(task->code), task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);
    if (task->code == STATE_WRITE) {
        task->code = STATE_CONNECTED;
        LuaTaskResume(task, 1);
    }
}

struct Task ** pushLuaNet(lua_State *L, struct Task * task) {
    struct Task **ptr = (struct Task **) lua_newuserdata(L, sizeof (*task));
    *ptr = task;
    luaL_getmetatable(L, LuaNet);
    lua_setmetatable(L, 2);
    return ptr;
}

struct Task * toLuaNet(lua_State *L, int index) {
    struct Task *task = (struct Task *) lua_touserdata(L, index);
    if (task == NULL) luaL_typerror(L, index, "Task");
    return task;
}

/*
static int LuaNet_gc(lua_State *L) {
    struct Task *task = toLuaNet(L, 1);
    if (task->isSub) {
        stack(L);
        printf("goodbye Task(%p:%d:%p)\n", task, task->LObjId, lua_touserdata(L, 1));
        lua_remove(L, 1);
    }
    return 0;
}
 */

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

static int LuaNet_tostring(lua_State * L) {
    lua_pushfstring(L, "API: %p", lua_touserdata(L, 1));
    return 1;
}

static int LuaNetHMAC(lua_State *L) {

    HMAC_CTX hmactx;
    const EVP_MD *md;
    unsigned char md_value[EVP_MAX_MD_SIZE];
    char *algorithm = NULL;
    u_char *digest = NULL;
    u_char *cur = NULL;
    unsigned int md_len = 0;
    unsigned int i = 0;
    size_t msg_len = 0;
    if (!lua_isstring(L, 1) or !lua_isstring(L, 2) or !lua_isstring(L, 3)) {
        lua_pushfstring(L, "Need Three Arguments");
        return lua_error(L);
    }
    // Get the algorithm name from the closure
    algorithm = (char *) lua_tostring(L, 1);

    // Get the digest
    OpenSSL_add_all_digests();
    md = EVP_get_digestbyname(algorithm);
    if (!md) {
        lua_pushfstring(L, "No such hash algorithm: %s", algorithm);
        return lua_error(L);
    }

    // Initialise the hash context
    HMAC_CTX_init(&hmactx);

    cur = (u_char *) lua_tolstring(L, 2, &msg_len);
    HMAC_Init_ex(&hmactx, cur, msg_len, md, NULL);

    cur = (u_char *) lua_tolstring(L, 3, &msg_len);
    HMAC_Update(&hmactx, cur, msg_len);

    HMAC_Final(&hmactx, md_value, &md_len);
    HMAC_CTX_cleanup(&hmactx);

    msg_len = 1 + 2 * md_len;
    cur = digest = (u_char *) malloc(msg_len);
    for (i = 0; i < md_len; i++) {
        snprintf((char *) cur, 3, "%02x", md_value[i]);
        cur = cur + 2;
    }
    cur[0] = '\0';
    // Push the result onto the stack
    lua_pushstring(L, (char *) digest);
    free(digest);
    return (1);
}

static int LuaNetHash(lua_State * L) {
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

static int LuaNetBase64Encode(lua_State * L) {
    char *ptr, *encoded;
    int inlen = 40000, outlen;

    ptr = (char *) lua_tolstring(L, -1, (size_t *) & inlen);
    outlen = (inlen << 1);
    encoded = getNulledMemory(outlen);
    base64_encode(ptr, inlen, encoded, &outlen);
    lua_pushlstring(L, encoded, outlen);

    free(encoded);
    return (1);
}

static int LuaNetBase64Decode(lua_State * L) {
    char *ptr, *decoded;
    int inlen = 40000, outlen;
    ptr = (char *) lua_tolstring(L, -1, (size_t *) & inlen);
    outlen = inlen;
    decoded = getNulledMemory(outlen);
    base64_decode((const u_char *) ptr, (u_char *) decoded, &outlen);
    lua_pushlstring(L, decoded, outlen);

    free(decoded);

    return (1);
}

static int LuaNetIconv(lua_State * L) {
    char *fromStringPtr, *fromString, *fromCharset, *toString, *toStringPtr;
    iconv_t Iconv;
    size_t lenIn = 40000, lenOut, i;
    fromCharset = (char *) lua_tostring(L, -1);
    fromStringPtr = fromString = (char *) lua_tolstring(L, -2, (size_t*) & lenIn);

    lenOut = lenIn << 2;
    toStringPtr = toString = getNulledMemory(lenOut);
    Iconv = iconv_open("UTF-8//IGNORE", fromCharset);
    i = iconv(Iconv,
            &fromStringPtr, &lenIn,
            &toStringPtr, &lenOut
            );
    iconv_close(Iconv);

    if (i>-1) {
        lua_pushlstring(L, toString, lenOut);
    } else {
        lua_pushnil(L);
    }

    free(toString);
    return (1);
}

static int LuaNetConnect(lua_State * L) {
    struct Task *task, *subtask;
    struct sockaddr_in sa;
    struct stTCPUDPInfo *poll;



    lua_pushthread(L);

    lua_gettable(L, LUA_REGISTRYINDEX);
    task = lua_touserdata(L, -1);
    struct struct_LuaTask *luaTask = (struct struct_LuaTask *) task->ptr;
    lua_pop(L, 1);



    poll = (struct stTCPUDPInfo *) task->poll;


    if (lua_gettop(L) == 1) {
        debug("%d %s", task->LObjId, task->isSub ? "is sub" : "");
        pushLuaNet(L, task);
    } else if (lua_gettop(L) == 3) {
        debug("%d %s", task->LObjId, task->isSub ? "is sub" : "");
        if (lua_type(L, 2) != LUA_TSTRING) {
            lua_pop(L, 2);
            return 0;
        }
        const char *ip;
        subtask = getNulledMemory(sizeof (*task));
        subtask->poll = getNulledMemory(sizeof (struct stTCPUDPInfo));
        poll = (struct stTCPUDPInfo *) subtask->poll;
        subtask->ptr = task->ptr;
        subtask->isSub = 1;
        subtask->isEnd = 1;

        LuaNetListEntryPtr = getNulledMemory(sizeof (struct LuaNetListEntry));
        LuaNetListEntryPtr->task = subtask;
        LIST_INSERT_HEAD(&luaTask->LuaNetListHead, LuaNetListEntryPtr, entries);

        ip = lua_tostring(L, 2);
        inet_aton(ip, (struct in_addr *) &(subtask->Record.IP));
        subtask->Record.Port = (int) lua_tonumber(L, 3);
        subtask->Record.TimeOut = task->Record.TimeOut;


        sa.sin_addr = *((struct in_addr *) & subtask->Record.IP);
        sa.sin_family = AF_INET;
        sa.sin_port = htons(subtask->Record.Port);

        bzero(&sa.sin_zero, 8);




        poll->bev = bufferevent_socket_new(luaTask->luaModule->base, -1, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_socket_connect(poll->bev, (struct sockaddr *) & sa, sizeof (sa));
        bufferevent_setcb(poll->bev, OnReadLuaTask, OnWriteLuaSubTask, OnErrorLuaSubTask, subtask);
        bufferevent_enable(poll->bev, EV_WRITE | EV_TIMEOUT);


        timerclear(&tv);
        if (task->Record.TimeOut > 1000) {
            tv.tv_sec = task->Record.TimeOut / 1000 / 2;
            tv.tv_usec = (task->Record.TimeOut % 1000)*1000 / 2;
        } else {
            tv.tv_usec = task->Record.TimeOut * 1000 / 2;
        }

        bufferevent_set_timeouts(poll->bev, &tv, &tv);




        subtask->code = STATE_CONNECTING;

        lua_pop(L, 2);
        pushLuaNet(L, subtask);
        debug("task->LobjId is %d and task->end is %d, task - %p", subtask->LObjId, subtask->isEnd, subtask);
        return lua_yield(L, 1);
    }


    return 1;
}

void timerLuaNetRead(int fd, short action, void *arg) {
    struct Task *task = (struct Task *) arg;
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    struct struct_LuaTask *luaTask = (struct struct_LuaTask *) task->ptr;
    struct evbuffer *buffer = EVBUFFER_INPUT(poll->bev);
    u_char *data = EVBUFFER_DATA(buffer);

    u_int len = EVBUFFER_LENGTH(buffer);
    bufferevent_disable(poll->bev, EV_READ);

    debug("id %d, Module %s for %s [%s:%d] %s",
            task->Record.LObjId,
            getModuleText(task->Record.ModType),
            task->Record.HostName,
            ipString(task->Record.IP),
            task->Record.Port,
            task->isSub ? "is sub" : "");

    task->isRead = 0;
    if (task->readedSize == 0) {
        lua_pushnil(luaTask->state);
    } else {
        lua_pushlstring(luaTask->state, (char *) data, len);
        task->readedSize = 0;
    }

    task->code = STATE_CONNECTED;

    evbuffer_drain(buffer, len);
    LuaTaskResume(task, 1);
}

static int LuaNetRead(lua_State * L) {
    struct Task *task = checkLuaNet(L, 1);
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    struct struct_LuaTask *luaTask = (struct struct_LuaTask *) task->ptr;


    debug("%d %p %p %s", task->LObjId, task, L, task->isSub ? "is sub" : "");

    task->code = STATE_READ;


    timerclear(&tv);
    if (task->Record.TimeOut > 1000) {
        tv.tv_sec = task->Record.TimeOut / 1000 / 2;
        tv.tv_usec = (task->Record.TimeOut % 1000)*1000;
    } else {
        tv.tv_usec = task->Record.TimeOut * 1000 / 2;
    }




    bufferevent_enable(poll->bev, EV_READ);
    bufferevent_set_timeouts(poll->bev, &tv, &tv);


    task->readedSize = 0;
    task->isRead = 1;
    event_assign(&task->read, luaTask->luaModule->base, -1, 0, timerLuaNetRead, task);
    evtimer_add(&task->read, &tv);
    return lua_yield(L, 1);
}

static int LuaNetWrite(lua_State * L) {
    int len = 40000;


    struct Task *task = checkLuaNet(L, 1);
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;

    const char *ptr = lua_tolstring(L, 2, (size_t *) & len);
    task->code = STATE_WRITE;
    debug("%d bytes %p %d %p %p %s", len, task, task->LObjId, task, L, task->isSub ? "is sub" : "");


    bufferevent_write(poll->bev, ptr, len);
    bufferevent_enable(poll->bev, EV_WRITE);

    return lua_yield(L, 0);
}

void timerLuaNetSleep(int fd, short action, void *arg) {
    struct Task *task = (struct Task *) arg;
    debug("id:%d", task->LObjId);

    ((struct struct_LuaTask *) task->ptr)->issleep = 0;
    LuaTaskResume(task, 0);
}

static int LuaNetSleep(lua_State * L) {
    if (lua_gettop(L) != 2) return 0;
    struct Task *task = checkLuaNet(L, 1);
    struct struct_LuaTask *luaTask = (struct struct_LuaTask *) task->ptr;
    timerclear(&tv);
    tv.tv_usec = lua_tonumber(L, 2);
    luaTask->issleep = 1;
    event_assign(&luaTask->sleep, luaTask->luaModule->base, -1, 0, timerLuaNetSleep, task);
    evtimer_add(&luaTask->sleep, &tv);
    return lua_yield(L, 0);
}

static int LuaNetError(lua_State * L) {
    struct Task *task = checkLuaNet(L, 1);
    debug("%d %s", task->LObjId, task->isSub ? "is sub" : "");
    if (!task->isSub) task->code = STATE_ERROR;
    return 0;
}

static int LuaNetDone(lua_State * L) {
    struct Task *task = checkLuaNet(L, 1);
    debug("%d %s", task->LObjId, task->isSub ? "is sub" : "");
    if (!task->isSub) task->code = STATE_DONE;
    return 0;
}

static int LuaNetClose(lua_State * L) {
    struct Task *task = checkLuaNet(L, 1);
    debug("%d %s", task->LObjId, task->isSub ? "is sub" : "");
    if (!task or !task->isSub) return 0;
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    struct struct_LuaTask *luaTask = (struct struct_LuaTask *) task->ptr;

    debug("\tReally-> id %d %p, Module %s for %s [%s:%d] [ms: %d] %s",
            task->LObjId,
            task,
            getModuleText(task->Record.ModType),
            task->Record.HostName,
            ipString(task->Record.IP),
            task->Record.Port,
            poll->DelayMS,
            task->isSub ? "is sub" : "");

    if (luaTask and luaTask->issleep) {
        luaTask->issleep = 0;
        evtimer_del(&luaTask->sleep);
    }

    LIST_FOREACH(LuaNetListEntryPtr, &luaTask->LuaNetListHead, entries) {
        if (task == LuaNetListEntryPtr->task) {
            if (LuaNetListEntryPtr->task->isRead) {
                LuaNetListEntryPtr->task->isRead = 0;
                evtimer_del(&LuaNetListEntryPtr->task->read);
            }

            if ((char *) ((struct stTCPUDPInfo *) (LuaNetListEntryPtr->task->poll))->bev) {

                bufferevent_free(((struct stTCPUDPInfo *) (LuaNetListEntryPtr->task->poll))->bev);
                ((struct stTCPUDPInfo *) (LuaNetListEntryPtr->task->poll))->bev = 0;
                ((struct stTCPUDPInfo *) (LuaNetListEntryPtr->task->poll))->fd = 0;
            }
            deleteTask(LuaNetListEntryPtr->task);
            LIST_REMOVE(LuaNetListEntryPtr, entries);
            free(LuaNetListEntryPtr);

            break;
        }
    }


    return 0;
}

void openLuaConnection(struct Task * task) {
    struct stTCPUDPInfo *poll = (struct stTCPUDPInfo *) task->poll;
    struct struct_LuaTask *luaTask = (struct struct_LuaTask *) task->ptr;
    struct struct_LuaModule *luaModule = getLua(task->Record.ModType);


    if (luaModule == NULL) {
        task->code = STATE_ERROR;
        task->callback(task);
        evtimer_del(&task->time_ev);
        deleteTask(task);
        task = 0;
        return;
    }
    luaTask->luaModule = luaModule;

    struct sockaddr_in sa;
#ifdef REPORT_TIMEOUT
    task->Record.Port = 12341;
#endif

    debug("%s:%d, LObjId = %d %s", ipString(task->Record.IP), task->Record.Port, task->LObjId, task->isSub ? "is sub" : "");

    gettimeofday(&poll->CheckDt, NULL);

    task->code = STATE_CONNECTING;

    sa.sin_addr = *((struct in_addr *) & task->Record.IP);
    sa.sin_family = AF_INET;

    sa.sin_port = htons(task->Record.Port);
    bzero(&sa.sin_zero, 8);


    poll->bev = bufferevent_socket_new(luaTask->luaModule->base, -1, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_socket_connect(poll->bev, (struct sockaddr *) & sa, sizeof (sa));
    bufferevent_setcb(poll->bev, OnReadLuaTask, OnWriteLuaTask, OnErrorLuaTask, task);
    bufferevent_enable(poll->bev, EV_WRITE | EV_TIMEOUT);


    timerclear(&tv);
    if (task->Record.TimeOut > 1000) {
        tv.tv_sec = task->Record.TimeOut / 1000;
        tv.tv_usec = (task->Record.TimeOut % 1000)*1000;
    } else {
        tv.tv_usec = task->Record.TimeOut * 1000;
    }

    bufferevent_set_timeouts(poll->bev, &tv, &tv);


};

void RunLuaTask(struct Task * task) {
    struct struct_LuaTask *luaTask;
    char *ptr;
    debug("id:%d %s", task->LObjId, task->isSub ? "is sub" : "");


    luaTask = (struct struct_LuaTask *) task->ptr;
    struct struct_LuaModule *luaModule = luaTask->luaModule;

    /*
        lua_newtable(thread);
        lua_newtable(thread);
        lua_pushliteral(thread, "__index");
        lua_pushvalue(thread, LUA_GLOBALSINDEX);
        lua_settable(thread, -3);
        lua_setmetatable(thread, -2);
        lua_replace(thread, LUA_GLOBALSINDEX);
     */


    luaTask->state = lua_newthread(luaModule->state);
    luaTask->ptask = task;
    luaTask->ref = luaL_ref(luaModule->state, LUA_REGISTRYINDEX);

    LIST_INIT(&luaTask->LuaNetListHead);


    lua_pushthread(luaTask->state);
    lua_pushlightuserdata(luaTask->state, task);
    lua_settable(luaTask->state, LUA_REGISTRYINDEX);

    lua_getglobal(luaTask->state, "main");

    lua_newtable(luaTask->state);
    int countKey, sizeKey, sizeSize, i, ii;
    ptr = luaTask->ptr;

#define checkByteIsDig(x)       if (x < 0x30 or x > 0x39) {            incStat(task->Record.ModType, STATE_ERROR);            return;        }
#define parseSize(to,from,idx)  for (to = 0, idx = sizeSize - 1; idx >= 0; idx--) {\
                                  checkByteIsDig(*from);\
                                  if (idx) to += (*from++ - 0x30)*10 * idx;\
                                  else to += *from++ - 0x30;\
                                }


    while (*ptr) {
        checkByteIsDig(ptr[0]);
        sizeSize = ptr[0] - 0x30;
        countKey = 0;
        ptr++;
        parseSize(countKey, ptr, i);

        checkByteIsDig(ptr[0]);
        sizeSize = ptr[0] - 0x30;
        ptr++;
        parseSize(sizeKey, ptr, i);

        lua_pushstring(luaTask->state, "method");
        lua_pushlstring(luaTask->state, ptr, sizeKey);
        lua_settable(luaTask->state, 2);

        lua_pushstring(luaTask->state, "data");
        lua_newtable(luaTask->state);
        ptr += sizeKey;

        for (i = 1; i < countKey; i++) {

            checkByteIsDig(ptr[0]);
            sizeSize = ptr[0] - 0x30;
            ptr++;

            parseSize(sizeKey, ptr, ii);
            lua_pushlstring(luaTask->state, ptr, sizeKey);
            lua_rawseti(luaTask->state, -2, i);
            ptr += sizeKey;
        }
        lua_settable(luaTask->state, -3);
    }
    // сохраняю
    //создаю таблицу zzz
    //заполняю её

    lua_pushstring(luaTask->state, "host");
    lua_pushstring(luaTask->state, task->Record.HostName);
    lua_settable(luaTask->state, 2);

    lua_pushstring(luaTask->state, "port");
    lua_pushinteger(luaTask->state, task->Record.Port);
    lua_settable(luaTask->state, 2);

    lua_pushstring(luaTask->state, "ip");
    lua_pushstring(luaTask->state, ipString(task->Record.IP));
    lua_settable(luaTask->state, 2);

#ifdef DEBUG
    lua_pushstring(luaTask->state, "debug");
    lua_pushnumber(luaTask->state, 1);
    lua_settable(luaTask->state, 2);
#endif
#ifdef REPORT_ERROR
    lua_pushstring(luaTask->state, "generror");
    lua_pushnumber(luaTask->state, 1);
    lua_settable(luaTask->state, 2);
#endif

    lua_pushstring(luaTask->state, "id");
    lua_pushinteger(luaTask->state, task->LObjId);
    lua_settable(luaTask->state, 2);

    lua_resume(luaTask->state, 1);
    if (cycles > 100) {
        int memuse = lua_gc(luaModule->state, LUA_GCCOUNT, 0);
        lua_gc(luaModule->state, LUA_GCCOLLECT, 0);
        debug("Memory used before gc = %d (%d), cycles %d", memuse, memuse - lua_gc(luaModule->state, LUA_GCCOUNT, 0), cycles);
        cycles = 0;
    }
    cycles++;



}

void timerLuaTask(int fd, short action, void *arg) {
    struct Task *task = (struct Task *) arg;
    debug("%d id:%d %s", action, task->LObjId, getStatusText(task->code));
    if (task->code) {
        closeLuaConnection(task);
    }

    if (task->isEnd != TRUE and task->Record.IP) {
        timerclear(&tv);

        if (task->Record.CheckPeriod and task->pServer->timeOfLastUpdate == task->timeOfLastUpdate) {
            tv.tv_sec = task->Record.CheckPeriod;
        } else {
            tv.tv_sec = 60;
            task->isEnd = TRUE;
        }
        evtimer_add(&task->time_ev, &tv);
        openLuaConnection(task);
    }
}

void initLUATester() {
    //    pthread_t threads;
    LoadLuaFromDisk();
    //    pthread_create(&threads, NULL, initLuaThread, NULL);
}

// gcc -I /usr/include/lua5.1/  curve25519-donna/curve25519-donna.c  -levent  -llua5.1 -lssl yxml.c tools.c test_lua.c -o test_lua && ./test_lua

