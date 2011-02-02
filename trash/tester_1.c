#include <netinet/ip_icmp.h>
#include "include.h"
#include <sys/queue.h>
#include <ev.h>

struct timeval tv;
static void *root = NULL;

int compare(const void *left, const void *right) {
    if (((struct Task *) left)->LObjId > ((struct Task *) right)->LObjId) return 1;
    if (((struct Task *) left)->LObjId < ((struct Task *) right)->LObjId) return -1;
    return 0;
}

struct Task * getTask(int LObjId) {
    struct Task *task, *ptr;
    task = malloc(sizeof (*task));
    bzero(task, sizeof (*task));
    task->LObjId = LObjId;
    ptr = *(struct Task **) tsearch((void *) task, &root, compare);
    if (ptr != task) {
        free(task);
        task = ptr;
    }
    return task;

};

void readFromNetworkICMPTester(int fd, short event, void *arg) {
    struct icmp *ping;
    char packet[100];
    u8 countPerIter = 0;
    int len = 100;
    int lObjId = 0;
    socklen_t salen = sizeof (struct sockaddr);
    struct sockaddr_in sa;
    bzero(&sa, salen);
    printf(cGREEN"ICMP READER"cEND" "cBLUE"%s"cEND"\n", getActionText(event));
    while (++countPerIter < 200 && (len = recvfrom(IcmpPool.fd, (char *) & packet, len, 0, (struct sockaddr *) & sa, (socklen_t *) & salen)) > 0) {
        if (len < 32) continue;
        ping = (struct icmp *) & packet;
        lObjId = *(u32 *) (packet + 28);

        LIST_FOREACH(IcmpEntryPtr, &IcmpHead, entries) {
            if (lObjId != IcmpEntryPtr->task->LObjId) continue;

            /*
                        printf("Eat LobjId->%d %d\n", IcmpEntryPtr->task->LObjId, len);
             */
            IcmpEntryPtr->task->code = 1;
            printf("LobjId->%d/%d, IP->%s\n", lObjId, IcmpEntryPtr->task->LObjId, ipString(sa.sin_addr.s_addr));
            printf("\tping->icmp_code=0x%08x\n", ping->icmp_code);
            printf("\tping->icmp_cksum=0x%08x\n", ping->icmp_cksum);
            hexPrint(packet, len);
            printf("IcmpEntryPtr->task->Record.CheckPeriod = %d\n", IcmpEntryPtr->task->Record.CheckPeriod);
            LIST_REMOVE(IcmpEntryPtr, entries);
            free(IcmpEntryPtr);
            break;
        }
    }

    /*
        LIST_FOREACH(IcmpEntryPtr, &IcmpHead, entries) {
            printf("Need LobjId->%d\n", IcmpEntryPtr->task->LObjId);
        }
     */
    if (LIST_EMPTY(&IcmpHead)) {
        event_del(&IcmpPool.ev);
        close(IcmpPool.fd);
        IcmpPool.fd = 0;
    }

}
int icmpOK = 0;
int icmpTIMEOUT = 0;

void runICMPTask(int fd, short action, void *arg) {
    struct Task *task = (struct Task *) arg;
    struct timeval tv;
    struct icmp *ping;
    struct sockaddr_in sa;
    char packet[12];
    printf(cGREEN"TASK"cEND" RUN -> id %d, Module %s for %s [%s:%d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);
    bzero(&packet, 12);
    memcpy(&packet[8], &task->LObjId, 4);

    ping = (struct icmp *) & packet;
    ping->icmp_type = ICMP_ECHO;
    ping->icmp_code = rand();
    ping->icmp_seq = rand();
    ping->icmp_cksum = in_cksum((u_short *) & packet, 12);

    sa.sin_addr.s_addr = (in_addr_t) task->Record.IP;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(0);
    bzero(&sa.sin_zero, 8);
    sendto(IcmpPool.fd, (char *) packet, 12, 0, (struct sockaddr *) & sa, sizeof (struct sockaddr));
    task->code = 2;
    IcmpEntryPtr = malloc(sizeof (struct IcmpListEntry)); /* Insert at the head. */
    IcmpEntryPtr->task = task;
    LIST_INSERT_HEAD(&IcmpHead, IcmpEntryPtr, entries);

}

/*
    struct msghdr hdr;
    hdr.msg_name
 */


void scheduleICMPTask(int fd, short action, void *arg) {
    struct Task *task = (struct Task *) arg;

    if (task->code == 2) {
        icmpTIMEOUT += 1;
    } else if (task->code == 1) {
        icmpOK += 1;
    }

    if (task->end == TRUE) {

        LIST_FOREACH(IcmpEntryPtr, &IcmpHead, entries) {
            if (task->LObjId != IcmpEntryPtr->task->LObjId) continue;
            LIST_REMOVE(IcmpEntryPtr, entries);
            free(IcmpEntryPtr);
            break;
        }
        if (LIST_EMPTY(&IcmpHead)) {
            event_del(&IcmpPool.ev);
            close(IcmpPool.fd);
            IcmpPool.fd = 0;
        }
        deleteICMPTask(task);
        return;
    }


    if (IcmpPool.fd == 0) {
        IcmpPool.fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        //        setsockopt(IcmpPool.fd, SOL_RAW, 1, ~(ICMP_ECHOREPLY | ICMP_DEST_UNREACH | ICMP_SOURCE_QUENCH | ICMP_REDIRECT | ICMP_TIME_EXCEEDED | ICMP_PARAMETERPROB), 4);
        setnb(IcmpPool.fd);
        event_set(&IcmpPool.ev, IcmpPool.fd, EV_READ | EV_PERSIST, readFromNetworkICMPTester, NULL);
        event_add(&IcmpPool.ev, NULL);
    }

 //   event_set(&task->ev, IcmpPool.fd, EV_WRITE, runICMPTask, task);
//    event_add(&task->ev, NULL);
    printf(cGREEN"TASK"cEND" SCHEDULE -> id %d, Module %s for %s [%s:%d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);

    timerclear(&tv);
    if (task->Record.CheckPeriod) {
        tv.tv_sec = task->Record.CheckPeriod;
    } else {
        task->end = TRUE;
        tv.tv_sec = 6;
    }
    evtimer_add(&task->time_ev, &tv);

}

inline void deleteICMPTask(struct Task * task) {
    printf(cGREEN"ICMP DELETE"cEND" "cBLUE"%d - %s"cEND"\n", task->LObjId, ipString(task->Record.IP));
    event_del(&task->ev);
    tdelete((void *) task, &root, compare);
    free(task);
};

/*
inline void runTaskResolv(int fd, short action, void *arg) {
    struct st_pool *pool = (struct st_pool *) arg;
    timerclear(&tv);
    tv.tv_sec = pool->Record.NextCheckDt;
    evtimer_add(&pool->ev, &tv);
}

void addTaskResolv(struct st_pool *pool) {
    printf(cGREEN"TASK"cEND" RESOLV NEW -> id %d, host %s\n", pool->Record.LObjId, pool.Record->HostName);
    evtimer_set(&pool->ev, runTask, pool);
};
 */

void addTask(struct _Tester_Cfg_Record * Record) {
    struct timeval tv;
    timerclear(&tv);
    struct Task *task = getTask(Record->LObjId);


    if (task->Record.LObjId) {
        printf(cGREEN"TASK"cEND" REPLACE -> id %d, Module %s for %s [%s:%d]\n", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port);
    } else {
        printf(cGREEN"TASK"cEND" NEW -> id %d, Module %s for %s [%s:%d]\n", Record->LObjId, getModuleText(Record->ModType), Record->HostName, ipString(Record->IP), Record->Port);
        tv.tv_sec = config.TimeDifference + Record->NextCheckDt;
        if (Record->ModType == MODULE_PING) {
            evtimer_set(&task->time_ev, scheduleICMPTask, task);
            evtimer_add(&task->time_ev, &tv);
        }
    }

    memcpy(&((task)->Record), Record, sizeof (*Record));



};

/*


for (IcmpEntryPtr = IcmpHead.lh_first; IcmpEntryPtr != NULL; IcmpEntryPtr = IcmpEntryPtr->entries.le_next) {
    printf("TaskID: %d\n", IcmpEntryPtr->taskID);
}

 */


void initTesterVar() {
    IcmpPool.fd = 0;
    LIST_INIT(&IcmpHead);
}


#define TEST
#ifdef TEST

int main() {
    event_init();
    initTesterVar();
#define COUNT 1

    struct _Tester_Cfg_Record RequestICMP[COUNT];
    int i = 0;
    u32 ip = 0;
    inet_aton("192.168.1.200", &ip);

    for (i = 0; i < COUNT; i++) {
        bzero(&RequestICMP[i], sizeof (struct _Tester_Cfg_Record));
        RequestICMP[i].CheckPeriod = 0;
        RequestICMP[i].IP = ip;
        RequestICMP[i].LObjId = i;
        RequestICMP[i].ModType = MODULE_PING;
        RequestICMP[i].Port = 0;
        RequestICMP[i].ResolvePeriod = 0;
        RequestICMP[i].NextCheckDt = 0;
    }

    for (i = 0; i < COUNT; i++) {
        addTask(&RequestICMP[i]);
    }


    event_dispatch();
    printf(cGREEN"\nICMP STAT"cEND" OK:%d , TIMEOUT: %d\n\n", icmpOK, icmpTIMEOUT);
    return 0;
}


#endif