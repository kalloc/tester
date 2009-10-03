#include "include.h"


struct timeval tv;
static void *rootForId = NULL;

int compareId(const void *left, const void *right) {
    if (((struct Task *) left)->LObjId > ((struct Task *) right)->LObjId) return 1;
    if (((struct Task *) left)->LObjId < ((struct Task *) right)->LObjId) return -1;
    return 0;
}

struct Task * searchTask(int LObjId, short flag) {
    struct Task *task, *ptr;
    task = getNulledMemory(sizeof (*task));
    task->LObjId = LObjId;
    ptr = *(struct Task **) tsearch((void *) task, &rootForId, compareId);
    if (ptr != task) {
        free(task);
        task = ptr;
    } else if (flag) {
        deleteTask(task);
        return NULL;
    }
    return task;
};

void deleteTask(struct Task *task) {
    if (task->LObjId) {
        tdelete((void *) task, &rootForId, compareId);
    }
    if (task->ptr and task->isSub == 0) {
        if (((struct struct_LuaTask *) task->ptr)->ptr) {
            free(((struct struct_LuaTask *) task->ptr)->ptr);
            ((struct struct_LuaTask *) task->ptr)->ptr = 0;
        }
        free(task->ptr);
        task->ptr = 0;
    }
    if (task->poll) {
        free(task->poll);
    }
    if (task->resolv) {
        evtimer_del(&task->resolv->timer);
        event_del(&task->resolv->ev);
        free(task->resolv);
    }
    free(task);
    task = 0;
}


#define addSubTaskResolv(method) \
        if (Record->ResolvePeriod) {\
            struct DNSTask *dnstask;\
            dnstask = getNulledMemory(sizeof (*dnstask)); \
            dnstask->task = task; \
            task->resolv = dnstask; \
            dnstask->role = method; \
            tv.tv_sec = 0; \
            evtimer_set(&dnstask->timer, timerDNSTask, task); \
            evtimer_add(&dnstask->timer, &tv); \
        }

#define addCallbackTimer(callback) \
        timerclear(&tv);\
        tv.tv_sec = firstStartTimer;\
        evtimer_set(&task->time_ev, callback, task);\
        evtimer_add(&task->time_ev, &tv);\
        task->timeRemainder=Record->NextCheckDt%Record->CheckPeriod;

#define modifyCallbackTimer(callback) \
        if (Record->NextCheckDt % Record->CheckPeriod != task->timeRemainder) {\
            debug("\tRemainter Modify, Old value - %d,New - %d,New Timer - %d",\
                task->timeRemainder,\
                Record->NextCheckDt % Record->CheckPeriod,\
                Record->CheckPeriod - (task->timeRemainder-(Record->NextCheckDt % Record->CheckPeriod)));\
            evtimer_del(&task->time_ev);\
            timerclear(&tv); \
            tv.tv_sec = Record->CheckPeriod - (task->timeRemainder-(Record->NextCheckDt % Record->CheckPeriod)); \
            evtimer_set(&task->time_ev, callback, task); \
            evtimer_add(&task->time_ev, &tv); \
            task->timeRemainder = Record->NextCheckDt % Record->CheckPeriod; \
        }


#define removeifChangeModType() \
        if (task->Record.ModType != Record->ModType) {\
            task->Record.CheckPeriod = 0;\
            task->isEnd = 1;\
            return;\
        }

void addTCPTask(Server *pServer, struct _Tester_Cfg_Record * Record, u32 firstStartTimer) {
    struct Task *task = createTask(Record->LObjId);

    if (task->Record.LObjId) {
        removeifChangeModType();
        modifyCallbackTimer(timerTCPTask);
    } else {

        debug("id %d, Module %s for %s [%s:%d]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port);
        task->pServer = pServer;
        task->poll = getNulledMemory(sizeof (struct stTCPUDPInfo));
        task->callback = addTCPReport;
        addCallbackTimer(timerTCPTask);
        addSubTaskResolv(DNS_RESOLV);
    }
    task->timeOfLastUpdate = task->pServer->timeOfLastUpdate;
    memcpy(&((task)->Record), Record, sizeof (*Record));
};

void addICMPTask(Server *pServer, struct _Tester_Cfg_Record * Record, u32 firstStartTimer) {
    struct Task *task = createTask(Record->LObjId);

    if (task->Record.LObjId) {
        removeifChangeModType();
        modifyCallbackTimer(timerICMPTask);
    } else {
        debug("id %d, Module %s for %s [%s:%d]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port);
        task->pServer = pServer;
        task->poll = getNulledMemory(sizeof (struct stTCPUDPInfo));
        task->callback = addICMPReport;
        addCallbackTimer(timerICMPTask);
        addSubTaskResolv(DNS_RESOLV);
    }
    task->timeOfLastUpdate = task->pServer->timeOfLastUpdate;
    memcpy(&((task)->Record), Record, sizeof (*Record));
};

void addLuaTask(Server *pServer, struct _Tester_Cfg_Record * Record, u32 firstStartTimer, char *data) {
    struct Task *task = createTask(Record->LObjId);

    if (task->Record.LObjId) {
        removeifChangeModType();
        modifyCallbackTimer(timerLuaTask);

        debug("REPLACE LUA -> id %d, Module %s for %s [%s:%d] [config %s]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port, ((struct struct_LuaTask *) task->ptr)->ptr);

        if (Record->ConfigLen != task->Record.ConfigLen) {
            free(((struct struct_LuaTask *) task->ptr)->ptr);
            ((struct struct_LuaTask *) task->ptr)->ptr = getNulledMemory(Record->ConfigLen + 1);
        }
    } else {
        task->pServer = pServer;
        task->poll = getNulledMemory(sizeof (struct stTCPUDPInfo));
        task->ptr = getNulledMemory(sizeof (struct struct_LuaTask));
        ((struct struct_LuaTask *) task->ptr)->ptr = getNulledMemory(Record->ConfigLen + 1);
        task->callback = addTCPReport;
        addCallbackTimer(timerLuaTask);
        addSubTaskResolv(DNS_RESOLV);
        debug("NEW LUA -> id %d, Module %s for %s [%s:%d] [config %s]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port, ((struct struct_LuaTask *) task->ptr)->ptr);
    }
    memcpy(&((task)->Record), Record, sizeof (*Record));
    task->timeOfLastUpdate = task->pServer->timeOfLastUpdate;
    memcpy(((struct struct_LuaTask *) task->ptr)->ptr, data, Record->ConfigLen);

};

void addDNSTask(Server *pServer, struct _Tester_Cfg_Record * Record, u32 firstStartTimer, char *data) {
    struct Task *task = createTask(Record->LObjId);

    if (task->Record.LObjId) {
        removeifChangeModType();
        modifyCallbackTimer(timerDNSTask);
        debug("REPLACE DNS -> id %d, Module %s for %s [%s:%d] [config %s]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port, task->ptr);
        if (Record->ConfigLen != task->Record.ConfigLen) {
            free(((struct struct_LuaTask *) task->ptr)->ptr);
            ((struct struct_LuaTask *) task->ptr)->ptr = getNulledMemory(Record->ConfigLen + 1);
        }
    } else {


        task->pServer = pServer;
        task->poll = getNulledMemory(sizeof (struct DNSTask));
        task->ptr = getNulledMemory(Record->ConfigLen + 1);
        task->callback = addTCPReport;
        addCallbackTimer(timerDNSTask);
        addSubTaskResolv(DNS_GETNS);
        debug("NEW DNS -> id %d, Module %s for %s [%s:%d] [config %s]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port, task->ptr);
    }
    memcpy(&((task)->Record), Record, sizeof (*Record));
    task->timeOfLastUpdate = task->pServer->timeOfLastUpdate;
    memcpy(task->ptr, data, Record->ConfigLen);

};

void initTester() {
    initICMPTester();
    initTCPTester();
    initDNSTester();
    initLUATester();
}
//#define TEST_PING
//#define TEST_TCP
//#define TEST_HTTP2

#ifdef TEST_ALL
#define TEST_FTP
#define TEST_HTTP
#define TEST_POP
#define TEST_SMTP
#endif
//#define TESTER

#ifdef TESTER

int main(int arg, char **argv) {
    initMainVars();

    openConfiguration("config.xml");

    initTester();
    u32 i = 0;
    u32 ip = 0;

#ifndef COUNT
#define COUNT 1
#endif
#ifndef CHECKPERIOD
#define CHECKPERIOD 0
#endif
#ifdef TEST_PING
    struct _Tester_Cfg_Record RequestICMP[COUNT];
    //inet_aton("195.42.186.21", (struct in_addr *) & ip);
    inet_aton("192.168.1.225", (struct in_addr *) & ip);
    //inet_aton("89.188.104.250", (struct in_addr *) & ip);

    for (i = 0; i < COUNT; i++) {
        bzero(&RequestICMP[i], sizeof (struct _Tester_Cfg_Record));
        RequestICMP[i].CheckPeriod = CHECKPERIOD;
        RequestICMP[i].IP = ip;
        RequestICMP[i].LObjId = 30000 + i;
        RequestICMP[i].ModType = MODULE_PING;
        RequestICMP[i].Port = 0;
        RequestICMP[i].TimeOut = 1000;
        RequestICMP[i].ResolvePeriod = 0;
        RequestICMP[i].NextCheckDt = 0;
    }

    for (i = 0; i < COUNT; i++) {
        addTCPTask(&RequestICMP[i]);
    }
#endif
#ifdef TEST_TCP
    struct _Tester_Cfg_Record RequestTCP[COUNT];
    inet_aton("192.168.1.225", (struct in_addr *) & ip);
    //inet_aton("213.248.62.7", &ip);

    for (i = 0; i < COUNT; i++) {
        bzero(&RequestTCP[i], sizeof (struct _Tester_Cfg_Record));
        RequestTCP[i].CheckPeriod = CHECKPERIOD;
        RequestTCP[i].IP = ip;
        RequestTCP[i].LObjId = 20000 + i;
        RequestTCP[i].ModType = MODULE_TCP_PORT;
        RequestTCP[i].Port = 80;
        RequestTCP[i].ResolvePeriod = 0;
        RequestTCP[i].TimeOut = 1;
        RequestTCP[i].NextCheckDt = 0;
    }

    for (i = 0; i < COUNT; i++) {
        addTCPTask(&RequestTCP[i]);
    }
#endif
#ifdef TEST_HTTP
    struct _Tester_Cfg_Record RequestHTTP[COUNT];
    //inet_aton("192.168.1.225", (struct in_addr *) & ip);
    //inet_aton("89.188.104.250", (struct in_addr *) & ip);
    //inet_aton("77.88.21.11", (struct in_addr *) & ip);
    inet_aton("213.248.62.4", (struct in_addr *) & ip);

    for (i = 0; i < COUNT; i++) {
        bzero(&RequestHTTP[i], sizeof (struct _Tester_Cfg_Record));
        RequestHTTP[i].CheckPeriod = CHECKPERIOD;
        RequestHTTP[i].IP = 0;
        RequestHTTP[i].LObjId = 10000 + i;
        RequestHTTP[i].ModType = MODULE_HTTP;
        RequestHTTP[i].Port = 80;
        RequestHTTP[i].TimeOut = 100;
        RequestHTTP[i].ConfigLen = sizeof ("1317chksize18test.php147000");
        RequestHTTP[i].ResolvePeriod = 60;
        RequestHTTP[i].NextCheckDt = 1;
        snprintf((char *) & RequestHTTP[i].HostName, TESTER_SQL_HOST_NAME_LEN, "n1ck.name");
    }

    for (i = 0; i < COUNT; i++) {
        addLuaTask(& RequestHTTP[i], "1317chksize18test.php147000");
    }
#endif
#ifdef TEST_POP
    struct _Tester_Cfg_Record RequestPOP[COUNT];
    inet_aton("213.248.62.7", (struct in_addr *) & ip);

    for (i = 0; i < COUNT; i++) {
        bzero(&RequestPOP[i], sizeof (struct _Tester_Cfg_Record));
        RequestPOP[i].CheckPeriod = CHECKPERIOD;
        RequestPOP[i].IP = ip;
        RequestPOP[i].LObjId = 40000 + i;
        RequestPOP[i].ModType = MODULE_POP;
        RequestPOP[i].Port = 110;
        RequestPOP[i].ResolvePeriod = 0;
        RequestPOP[i].NextCheckDt = 0;
        snprintf((char *) & RequestPOP[i].HostName, TESTER_SQL_HOST_NAME_LEN, "localhost");
    }

    for (i = 0; i < COUNT; i++) {
        addLuaTask(&RequestPOP[i], "1418chkwords14HTTP15nginx");
    }
#endif
#ifdef TEST_FTP
    struct _Tester_Cfg_Record RequestFTP[COUNT];
    inet_aton("213.248.62.7", (struct in_addr *) & ip);

    for (i = 0; i < COUNT; i++) {
        bzero(&RequestFTP[i], sizeof (struct _Tester_Cfg_Record));
        RequestFTP[i].CheckPeriod = CHECKPERIOD;
        RequestFTP[i].IP = ip;
        RequestFTP[i].LObjId = 50000 + i;
        RequestFTP[i].ModType = MODULE_FTP;
        RequestFTP[i].Port = 21;
        RequestFTP[i].TimeOut = 1000;
        RequestFTP[i].ConfigLen = sizeof ("1513put210solotester181234123411/15file2");
        RequestFTP[i].ResolvePeriod = 0;
        RequestFTP[i].NextCheckDt = 0;
        snprintf((char *) & RequestFTP[i].HostName, TESTER_SQL_HOST_NAME_LEN, "localhost");
    }

    for (i = 0; i < COUNT; i++) {
        addLuaTask(& RequestFTP[i], "1513put210solotester181234123411/15file2");
    }
#endif
#ifdef TEST_SMTP
    struct _Tester_Cfg_Record RequestSMTP[COUNT];
    inet_aton("213.248.62.7", (struct in_addr *) & ip);

    for (i = 0; i < COUNT; i++) {
        bzero(&RequestSMTP[i], sizeof (struct _Tester_Cfg_Record));
        RequestSMTP[i].CheckPeriod = CHECKPERIOD;
        RequestSMTP[i].IP = ip;
        RequestSMTP[i].LObjId = 60000 + i;
        RequestSMTP[i].ModType = MODULE_SMTP;
        RequestSMTP[i].Port = 25;
        //RequestSMTP[i].TimeOut = 50000;
        RequestSMTP[i].ResolvePeriod = 60;
        RequestSMTP[i].NextCheckDt = 0;
        RequestSMTP[i].ConfigLen = sizeof ("1314send220solotester@n1ck.name1812341234");
        snprintf((char *) & RequestSMTP[i].HostName, TESTER_SQL_HOST_NAME_LEN, "n1ck.name");
    }

    for (i = 0; i < COUNT; i++) {
        addLuaTask(& RequestSMTP[i], "1314send220solotester@n1ck.name1812341234");
    }
#endif
    event_dispatch();


#ifdef TEST_HTTP
    printf(cGREEN"\nMODULE_HTTP STAT"cEND" OK:%d , TIMEOUT: %d, ERROR: %d\n\n", config.stat[MODULE_HTTP].OK, config.stat[MODULE_HTTP].TIMEOUT, config.stat[MODULE_HTTP].ERROR);
#endif

#ifdef TEST_SMTP
    printf(cGREEN"\nMODULE_SMTP STAT"cEND" OK:%d , TIMEOUT: %d, ERROR: %d\n\n", config.stat[MODULE_SMTP].OK, config.stat[MODULE_SMTP].TIMEOUT, config.stat[MODULE_SMTP].ERROR);
#endif

#ifdef TEST_FTP
    printf(cGREEN"\nMODULE_FTP STAT"cEND" OK:%d , TIMEOUT: %d, ERROR: %d\n\n", config.stat[MODULE_FTP].OK, config.stat[MODULE_FTP].TIMEOUT, config.stat[MODULE_FTP].ERROR);
#endif

#ifdef TEST_POP
    printf(cGREEN"\nMODULE_POP STAT"cEND" OK:%d , TIMEOUT: %d, ERROR: %d\n\n", config.stat[MODULE_POP].OK, config.stat[MODULE_POP].TIMEOUT, config.stat[MODULE_POP].ERROR);
#endif

#ifdef TEST_PING
    printf(cGREEN"\nMODULE_PING STAT"cEND" OK:%d , TIMEOUT: %d, ERROR: %d\n\n", config.stat[MODULE_PING].OK, config.stat[MODULE_PING].TIMEOUT, config.stat[MODULE_PING].ERROR);
#endif

#ifdef TEST_TCP
    printf(cGREEN"\nMODULE_TCP_PORT STAT"cEND" OK:%d , TIMEOUT: %d, ERROR: %d\n\n", config.stat[MODULE_TCP_PORT].OK, config.stat[MODULE_TCP_PORT].TIMEOUT, config.stat[MODULE_TCP_PORT].ERROR);
#endif

    event_base_free(base);

    return 0;
}


#endif