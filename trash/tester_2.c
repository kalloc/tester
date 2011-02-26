#include <netinet/ip_icmp.h>
#include "include.h"
#include <sys/queue.h>


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
    printf(cGREEN"ICMP READ\n"cEND);
/*
    LIST_FOREACH(IcmpEntryPtr, &IcmpReadHead, entries) {
        printf("Wait for read LobjId->%d\n", IcmpEntryPtr->task->LObjId);
    }
*/

    if (LIST_EMPTY(&IcmpReadHead)&&LIST_EMPTY(&IcmpWriteHead)) {
        event_del(&IcmpPool.read_ev);
        return;
    }

    struct icmp *ping;
    struct Task *task;
    char packet[100];
    u8 countPerIter = 0;
    int len = 100;
    int lObjId = 0;
    socklen_t salen = sizeof (struct sockaddr);
    struct sockaddr_in sa;
    bzero(&sa, salen);
    while (++countPerIter < 200 && (len = recvfrom(IcmpPool.fd, (char *) & packet, len, 0, (struct sockaddr *) & sa, (socklen_t *) & salen)) > 0) {
        if (len < 32) continue;
        ping = (struct icmp *) & packet;
        lObjId = *(u32 *) (packet + 28);

        LIST_FOREACH(IcmpEntryPtr, &IcmpReadHead, entries) {
            if (lObjId != IcmpEntryPtr->task->LObjId) continue;
            task = IcmpEntryPtr->task;
            task->code = EV_READ;
            /*
                                    printf("LobjId->%d/%d, IP->%s\n", lObjId, IcmpEntryPtr->task->LObjId, ipString(sa.sin_addr.s_addr));
                                    printf("\tping->icmp_code=0x%08x\n", ping->icmp_code);
                                    printf("\tping->icmp_cksum=0x%08x\n", ping->icmp_cksum);
                                    printf("IcmpEntryPtr->task->Record.CheckPeriod = %d\n", IcmpEntryPtr->task->Record.CheckPeriod);
             */
            LIST_REMOVE(IcmpEntryPtr, entries);
            free(IcmpEntryPtr);
            if (task->end) {
                scheduleICMPTask(NULL, EV_TIMEOUT, task);
            }
            break;
        }
    }
    if (LIST_EMPTY(&IcmpReadHead)&&LIST_EMPTY(&IcmpWriteHead)) {
        event_del(&IcmpPool.read_ev);
        return;
    }

}
int icmpOK = 0;
int icmpTIMEOUT = 0;

void writeToNetworkICMPTester(int fd, short action, void *arg) {
    struct timeval tv;
    struct icmp *ping;
    struct sockaddr_in sa;
    char packet[12];

    printf(cGREEN"ICMP WRITE\n"cEND);

    LIST_FOREACH(IcmpEntryPtr, &IcmpWriteHead, entries) {
        printf(cGREEN"TASK"cEND" RUN -> id %d, Module %s for %s [%s:%d]\n", IcmpEntryPtr->task->Record.LObjId, getModuleText(IcmpEntryPtr->task->Record.ModType), IcmpEntryPtr->task->Record.HostName, ipString(IcmpEntryPtr->task->Record.IP), IcmpEntryPtr->task->Record.Port);
        bzero(&packet, 12);
        memcpy(&packet[8], &IcmpEntryPtr->task->LObjId, 4);

        ping = (struct icmp *) & packet;
        ping->icmp_type = ICMP_ECHO;
        ping->icmp_code = rand();
        ping->icmp_seq = rand();
        ping->icmp_cksum = in_cksum((u_short *) & packet, 12);

        sa.sin_addr.s_addr = (in_addr_t) IcmpEntryPtr->task->Record.IP;
        sa.sin_family = AF_INET;
        sa.sin_port = htons(0);
        bzero(&sa.sin_zero, 8);
        sendto(IcmpPool.fd, (char *) packet, 12, 0, (struct sockaddr *) & sa, sizeof (struct sockaddr));
        LIST_REMOVE(IcmpEntryPtr, entries);
        LIST_INSERT_HEAD(&IcmpReadHead, IcmpEntryPtr, entries);
        break;
    }

/*
    LIST_FOREACH(IcmpEntryPtr, &IcmpWriteHead, entries) {
        printf("Wait for write LobjId->%d\n", IcmpEntryPtr->task->LObjId);
    }
*/

    if (!LIST_EMPTY(&IcmpWriteHead)) {
        event_add(&IcmpPool.write_ev, NULL);
        return;
    }
}

/*
    struct msghdr hdr;
    hdr.msg_name
 */


void scheduleICMPTask(int fd, short action, void *arg) {
    struct Task *task = (struct Task *) arg;

    if (task->code == EV_TIMEOUT) {
        icmpTIMEOUT += 1;
    } else if (task->code == EV_READ) {
        icmpOK += 1;
    }

    if (task->end == TRUE) {
        deleteICMPTask(task);
        return;
    }
    printf(cGREEN"TASK"cEND" SCHEDULE -> id %d, Module %s for %s [%s:%d]\n", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);


    if (IcmpPool.fd == 0) {
        IcmpPool.fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        setnb(IcmpPool.fd);
        event_set(&IcmpPool.read_ev, IcmpPool.fd, EV_READ | EV_PERSIST, readFromNetworkICMPTester, NULL);
        event_add(&IcmpPool.read_ev, NULL);
        event_set(&IcmpPool.write_ev, IcmpPool.fd, EV_WRITE, writeToNetworkICMPTester, NULL);
        event_add(&IcmpPool.write_ev, NULL);
    }


    IcmpEntryPtr = malloc(sizeof (struct IcmpListEntry)); /* Insert at the head. */
    IcmpEntryPtr->task = task;
    LIST_INSERT_HEAD(&IcmpWriteHead, IcmpEntryPtr, entries);
    task->code = EV_TIMEOUT;

    timerclear(&tv);
    if (task->Record.CheckPeriod) {
        tv.tv_sec = task->Record.CheckPeriod;
    } else {
        task->end = TRUE;
        tv.tv_sec = 5;
    }
    evtimer_add(&task->time_ev, &tv);

}

inline void deleteICMPTask(struct Task * task) {
    printf(cGREEN"ICMP DELETE"cEND" "cBLUE"%d - %s"cEND"\n", task->LObjId, ipString(task->Record.IP));
    if (task->code == EV_TIMEOUT) {

        LIST_FOREACH(IcmpEntryPtr, &IcmpReadHead, entries) {
            if (task->LObjId != IcmpEntryPtr->task->LObjId) continue;
            LIST_REMOVE(IcmpEntryPtr, entries);
            free(IcmpEntryPtr);
            break;
        }

    }
    evtimer_del(&task->time_ev);
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
    LIST_INIT(&IcmpReadHead);
    LIST_INIT(&IcmpWriteHead);
}


#define TEST
#ifdef TEST

int main(int arg, char **argv) {
    event_init();
    initTesterVar();
#define COUNT 500

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