#include "include.h"
#include <math.h>
extern struct event_base *mainBase;
struct timeval tv;
static void *rootForId = NULL;
struct DNSTask *dnstask;
struct struct_LuaTask *luaTask;

struct Task *task, *ptask;
char *ptr;
int isNewConfig = 0;
int isNewTimer = 0;
int isNewResolv = 0;

static int main() {

}

int compareId(const void *left, const void *right) {
    if (((struct Task *) left)->LObjId > ((struct Task *) right)->LObjId) return 1;
    if (((struct Task *) left)->LObjId < ((struct Task *) right)->LObjId) return -1;
    return 0;
}

struct Task * searchTask(int LObjId, short flag) {
    task = getNulledMemory(sizeof (*task));
    task->LObjId = LObjId;
    ptask = *(struct Task **) tsearch((void *) task, &rootForId, compareId);
    if (ptask != task) {
        free(task);
        task = ptask;
    } else if (flag) {
        deleteTask(task);
        return NULL;
    }
    return task;
};

void deleteTask(struct Task *task) {
    debug("id is %d%s%s%s",task->LObjId,task->isSub?", isSub":"",task->isEnd?", isEnd":"",task->isVerifyTask?", isVerifyTask":"");
    if (task->LObjId and task->isSub == 0) {
        tdelete((void *) task, &rootForId, compareId);
    }
    if (task->ptr and task->isSub == 0) {

        if (task->Record.ModType != MODULE_DNS and((struct struct_LuaTask *) task->ptr)->ptr) {
            ((struct struct_LuaTask *) task->ptr)->state = 0;
            free(((struct struct_LuaTask *) task->ptr)->ptr);
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

}


#define addSubTaskResolv(method) \
   if (isNewResolv) {\
        if (Record->ResolvePeriod or method != DNS_RESOLV) {\
            dnstask = getNulledMemory(sizeof (*dnstask)); \
            dnstask->task = task; \
            task->resolv = dnstask; \
            dnstask->role = method; \
            tv.tv_usec = tv.tv_sec = 0; \
            event_assign(&dnstask->timer, getResolvBase(), -1, 0, timerResolv, task);\
            evtimer_add(&dnstask->timer, &tv); \
        }\
    }


#define addTimer(callback,base) \
   if (isNewTimer) {\
        timerclear(&tv);\
        tv.tv_sec = Record->NextCheckDt ? Record->NextCheckDt - pServer->timeOfLastUpdate : 0;\
        event_assign(&task->time_ev, base?base:mainBase, -1, 0, callback, task);\
        evtimer_add(&task->time_ev, &tv);\
        task->timeShift=newShift;\
   }

/*
            tv.tv_usec=0;\
            tv.tv_sec = task->newTimer;\
            task->newTimer=0;\
            evtimer_del(&task->time_ev);\
            evtimer_set(&task->time_ev, callback, task);\
            evtimer_add(&task->time_ev, &tv);\
 */

#define modifyCallbackTimer(callback) \
            debug("Lobj %d NewRemainder = %d, OldRemainder = %d, prevNextCheckDt = %d, NextCheckDt = %d", Record->LObjId, newShift, task->timeShift, task->Record.NextCheckDt, Record->NextCheckDt);\
            if (newShift != task->timeShift and task->newTimer == 0 and task->isShiftActive == 0) {\
                if ((task->Record.NextCheckDt - pServer->timeOfLastUpdate) > config.minRecheckPeriod and (Record->NextCheckDt - pServer->timeOfLastUpdate) > config.minRecheckPeriod ) {\
                    evtimer_del(&task->time_ev);\
                    isNewTimer = 1;\
                } else {\
                    task->newTimer = (Record->CheckPeriod - (task->timeShift - newShift)) % Record->CheckPeriod;\
                    if (task->newTimer <= config.minRecheckPeriod) {\
                        task->newTimer += task->Record.CheckPeriod;\
                    }\
                    debug("\tAs Remainder Modify id %d, set Timer - %d, prevNextCheckDt - %d, NextCheckDt - %d ",\
                            Record->LObjId, task->newTimer,\
                            task->Record.NextCheckDt, Record->NextCheckDt);\
                }\
                task->timeShift=newShift;\
            }

#define removeifChangeTask() \
        if (task->Record.ModType != Record->ModType or task->Record.ModType != Record->ModType or strcmp(Record->HostName,task->Record.HostName)) {\
            task->Record.CheckPeriod = 0;\
            setEnd(task);\
            pthread_mutex_unlock(mutex);\
            return;\
        }

void _addTCPTask(Server *pServer, struct _Tester_Cfg_Record * Record, int newShift, int isVerifyTask) {
    debug("mutex_lock")
    static pthread_mutex_t *mutex = NULL;
    if (!mutex) {
        mutex = calloc(1, sizeof (*mutex));
        pthread_mutex_init(mutex, NULL);
    }
    pthread_mutex_lock(mutex);
    isNewTimer = 0;
    isNewResolv = 0;

    task = createTask(Record->LObjId);

    if (task->Record.LObjId) {
        removeifChangeTask();
        modifyCallbackTimer(timerTCPTask);
        debug("REPLACE TCP id %d, Module %s for %s [%s:%d]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port);

    } else {

        task->pServer = pServer;
        task->poll = getNulledMemory(sizeof (struct stTCPUDPInfo));
        task->callback = addTCPReport;
        if(isVerifyTask)  task->isVerifyTask = 1;
        isNewTimer = 1;
        isNewResolv = 1;
        debug("NEW TCP id %d, Module %s for %s [%s:%d]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port);

    }
    task->timeOfLastUpdate = task->pServer->timeOfLastUpdate;
    memcpy(&((task)->Record), Record, sizeof (*Record));

    addSubTaskResolv(DNS_RESOLV);
    addTimer(timerTCPTask, getTCPBase());
    debug("mutex_unlock")
    pthread_mutex_unlock(mutex);
};

void _addICMPTask(Server *pServer, struct _Tester_Cfg_Record * Record, int newShift,int isVerifyTask) {
    debug("mutex_lock")
    static pthread_mutex_t *mutex = NULL;
    if (!mutex) {
        mutex = calloc(1, sizeof (*mutex));
        pthread_mutex_init(mutex, NULL);
    }
    pthread_mutex_lock(mutex);
    isNewTimer = 0;
    isNewResolv = 0;

    task = createTask(Record->LObjId);

    if (task->Record.LObjId) {
        removeifChangeTask();
        modifyCallbackTimer(timerICMPTask);
        debug("REPLACE ICMP id %d, Module %s for %s [%s:%d]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port);
    } else {
        task->pServer = pServer;
        task->poll = getNulledMemory(sizeof (struct stTCPUDPInfo));
        task->callback = addICMPReport;
        if(isVerifyTask)  task->isVerifyTask = 1;
        isNewTimer = 1;
        isNewResolv = 1;
        debug("NEW ICMP id %d, Module %s for %s [%s:%d]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port);
    }
    task->timeOfLastUpdate = task->pServer->timeOfLastUpdate;
    memcpy(&((task)->Record), Record, sizeof (*Record));


    addSubTaskResolv(DNS_RESOLV);
    addTimer(timerICMPTask, getICMPBase());
    debug("mutex_unlock")
    pthread_mutex_unlock(mutex);

}

void _addLuaTask(Server *pServer, struct _Tester_Cfg_Record * Record, int newShift,int isVerifyTask, char *data) {
    debug("mutex_lock")
    static pthread_mutex_t *mutex = NULL;
    if (!mutex) {
        mutex = calloc(1, sizeof (*mutex));
        pthread_mutex_init(mutex, NULL);
    }
    pthread_mutex_lock(mutex);

    isNewConfig = 0;
    isNewTimer = 0;
    isNewResolv = 0;
    task = createTask(Record->LObjId);
    if (task->Record.LObjId) {
        removeifChangeTask();
        modifyCallbackTimer(timerLuaTask);
        debug("TRY TO REPLACE LUA -> id %d, Module %s for %s [%s:%d] [config %s]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port, config);
        if (Record->ConfigLen != task->Record.ConfigLen) {
            debug("REPLACE LUA -> id %d, Module %s for %s [%s:%d] [config %s]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port, config);
            free(((struct struct_LuaTask *) task->ptr)->ptr);
            ((struct struct_LuaTask *) task->ptr)->ptr = getNulledMemory(Record->ConfigLen + 1);
            isNewConfig = 1;
        } else if (memcmp(data, ((struct struct_LuaTask *) task->ptr)->ptr, Record->ConfigLen)) {
            isNewConfig = 1;
        }
    } else {
        task->pServer = pServer;
        task->poll = getNulledMemory(sizeof (struct stTCPUDPInfo));
        task->callback = addTCPReport;
        if(isVerifyTask)  task->isVerifyTask = 1;

        task->ptr = getNulledMemory(sizeof (struct struct_LuaTask));
        ((struct struct_LuaTask *) task->ptr)->ptr = getNulledMemory(Record->ConfigLen + 1);
        isNewTimer = 1;
        isNewResolv = 1;
        isNewConfig = 1;
        debug("NEW LUA -> id %d, Module %s for %s [%s:%d] [config %s]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port, data);
    }
    luaTask = (struct struct_LuaTask *) task->ptr;

    memcpy(&((task)->Record), Record, sizeof (*Record));
    task->timeOfLastUpdate = task->pServer->timeOfLastUpdate;
    if (isNewConfig) memcpy(luaTask->ptr, data, Record->ConfigLen);

    addSubTaskResolv(DNS_RESOLV);
    addTimer(timerLuaTask, getLuaBase(Record->ModType));
    debug("mutex_unlock")
    pthread_mutex_unlock(mutex);
}

void _addDNSTask(Server *pServer, struct _Tester_Cfg_Record * Record, int newShift,int isVerifyTask, char *data) {
    debug("mutex_lock")
    static pthread_mutex_t *mutex = NULL;
    if (!mutex) {
        mutex = calloc(1, sizeof (*mutex));
        pthread_mutex_init(mutex, NULL);
    }
    pthread_mutex_lock(mutex);
    u_short countByte = 0, countCountByte = 0;
    isNewConfig = 0;
    isNewTimer = 0;
    isNewResolv = 0;

    task = createTask(Record->LObjId);

    if (task->Record.LObjId) {
        removeifChangeTask();
        modifyCallbackTimer(timerDNSTask);
        debug("REPLACE DNS -> id %d, Module %s for %s [%s:%d] [config %s]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port, data);
        if (Record->ConfigLen != task->Record.ConfigLen) {
            free(task->ptr);
            task->ptr = getNulledMemory(Record->ConfigLen + 1);
            isNewConfig = 1;
        } else if (memcmp(data, task->ptr, Record->ConfigLen)) {
            isNewConfig = 1;
        }

        if (isNewConfig) {
            if (inet_addr(Record->HostName) != -1) task->isHostAsIp = 1;
            else task->isHostAsIp = 0;
        }

    } else {
        if (inet_addr(Record->HostName) != -1) task->isHostAsIp = 1;
        else task->isHostAsIp = 0;
        if(isVerifyTask)  task->isVerifyTask = 1;

        task->pServer = pServer;
        dnstask = getNulledMemory(sizeof (*dnstask));
        dnstask->task = task;
        dnstask->role = DNS_TASK;
        task->poll = dnstask;
        task->ptr = getNulledMemory(Record->ConfigLen + 1);
        task->callback = addDNSReport;
        debug("NEW DNS -> id %d, Module %s for %s [%s:%d] [config %s]", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port, data);
        isNewConfig = 1;
        isNewResolv = 1;
        isNewTimer = 1;
    }
    memcpy(&((task)->Record), Record, sizeof (*Record));
    task->timeOfLastUpdate = task->pServer->timeOfLastUpdate;

    if (isNewConfig) {
        memcpy(task->ptr, data, Record->ConfigLen);
        ptr = task->ptr;
        dnstask = (struct DNSTask *) task->poll;
        if (ptr[1] != 0x33) {
            setEnd(task);
            goto end;
        }
        ptr += 2;

        if (!memcmp(ptr, "11a", 3)) {
            dnstask->taskType = T_A;
            ptr += 3;
        } else if (!memcmp(ptr, "15cname", 7)) {
            dnstask->taskType = T_CNAME;
            ptr += 7;
        } else if (!memcmp(ptr, "12ns", 4)) {
            dnstask->taskType = T_NS;
            ptr += 4;
        } else if (!memcmp(ptr, "12mx", 4)) {
            dnstask->taskType = T_MX;
            ptr += 4;
        } else if (!memcmp(ptr, "13ptr", 5)) {
            dnstask->taskType = T_PTR;
            ptr += 5;
        } else {
            setEnd(task);
            goto end;
        }

        countCountByte = ptr[0] - 0x30;

        ptr++;
        for (; countCountByte > 0; countCountByte--, ++ptr) {
            countByte += (ptr[0] - 0x30) * pow(10, countCountByte - 1);
        }
        for (dnstask->taskTTL = 0; countByte > 0; countByte--, ++ptr) {
            dnstask->taskTTL += (ptr[0] - 0x30) * pow(10, countByte - 1);
        }

        countCountByte = ptr[0] - 0x30;
        ptr++;
        for (dnstask->taskPatternLen = 0; countCountByte > 0; countCountByte--, ++ptr) {
            dnstask->taskPatternLen += (ptr[0] - 0x30) * pow(10, countCountByte - 1);
        }


        dnstask->taskPattern = ptr;
    }

end:
    addSubTaskResolv(DNS_GETNS);
    addTimer(timerDNSTask, getDNSBase());
    /*
                    11 a
                    14 7200
                    214 82.179.196.194
     */
    debug("mutex_unlock")
    pthread_mutex_unlock(mutex);

};

void initTester() {
    initICMPTester();
    initTCPTester();
    initDNSTester();
    initLUATester();
}
