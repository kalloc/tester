#define _GNU_SOURCE     /* Expose declaration of tdestroy() */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <sys/queue.h>


#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <err.h>
#include <event.h>
#include <time.h>
#include <search.h>
/*
 *
 *		misc define
 *
 */

#pragma pack(1)


#define OR              ||
#define AND             &&
#define or              ||
#define and             &&

#define cBLUE		"\e[1;34m"
#define cYELLOW		"\e[1;33m"
#define cGREEN		"\e[1;32m"
#define cRED		"\e[1;31m"
#define cEND		"\e[0m"
#define TRUE            1
#define FALSE           0
//hack (:
#define u8             u_int8_t
#define u16             u_int16_t
#define u32             u_int32_t
#define u64             u_int64_t

struct _Tester_Cfg_Record {
    u32 LObjId; // id объекта тестирования
    u32 IP; // ип объекта тестирования
};

struct Task {
    u32 LObjId;
    void * pool;
    int fd;
    struct event time_ev;
    int code;
    short end;
    struct _Tester_Cfg_Record Record;
};

struct IcmpPooler {
    int fd;
    struct event read_ev;
    struct event write_ev;
} IcmpPool;



LIST_HEAD(IcmpWriteListHead, IcmpListEntry) IcmpWriteHead;
struct IcmpWriteListHead *IcmpWriteHeadPtr; /* List head. */

LIST_HEAD(IcmpReadListHead, IcmpListEntry) IcmpReadHead;
struct IcmpReadListHead *IcmpReadHeadPtr; /* List head. */

struct IcmpListEntry {
    struct Task *task;
    LIST_ENTRY(IcmpListEntry) entries; /* List. */
} *IcmpEntryPtr;



struct timeval tv;
static void *root = NULL;

int in_cksum(u_short *addr, int len) {
    int nleft = len;
    u_short *w = addr;
    int sum = 0;
    u_short answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(u_char *) (&answer) = *(u_char *) w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
    sum += (sum >> 16); /* add carry */
    answer = ~sum; /* truncate to 16 bits */

    return (answer);
}




int compare(const void *left, const void *right) {
    if (((struct Task *) left)->LObjId > ((struct Task *) right)->LObjId) return 1;
    if (((struct Task *) left)->LObjId < ((struct Task *) right)->LObjId) return -1;
    return 0;
}

int setnb(int fd) {
    int flags;
    flags = fcntl(fd, F_GETFL);
    if (flags < 0) return flags;
        flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) return -1;
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
//    printf(cGREEN"ICMP READ\n"cEND);



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
            LIST_REMOVE(IcmpEntryPtr, entries);
            free(IcmpEntryPtr);
            if (task->end) {
                scheduleICMPTask(NULL, EV_TIMEOUT, task);
            }
            break;
        }
    }
    
    if (!LIST_EMPTY(&IcmpReadHead)) {
//    	printf("Wait for read LobjId->");
	LIST_FOREACH(IcmpEntryPtr, &IcmpReadHead, entries) {
//    	    printf("%d ", IcmpEntryPtr->task->LObjId);
	}
//	printf("\n");
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

//    printf(cGREEN"ICMP WRITE\n"cEND);

    LIST_FOREACH(IcmpEntryPtr, &IcmpWriteHead, entries) {
//        printf(cGREEN"TASK"cEND" RUN -> id %d\n", IcmpEntryPtr->task->Record.LObjId);
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

/*    if (!LIST_EMPTY(&IcmpWriteHead)) {
    	printf("Wait for write LobjId->");
	LIST_FOREACH(IcmpEntryPtr, &IcmpWriteHead, entries) {
    	    printf("%d ", IcmpEntryPtr->task->LObjId);
	}
	printf("\n");
    }
*/

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
//    printf(cGREEN"TASK"cEND" SCHEDULE -> id %d\n", task->Record.LObjId);


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
    task->end = TRUE;
    tv.tv_sec = 10;
    evtimer_add(&task->time_ev, &tv);

}

inline void deleteICMPTask(struct Task * task) {
    //printf(cGREEN"ICMP DELETE"cEND" "cBLUE"%d"cEND"\n", task->LObjId);
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



void addTask(struct _Tester_Cfg_Record * Record) {
    struct timeval tv;
    timerclear(&tv);
    struct Task *task = getTask(Record->LObjId);
//    printf(cGREEN"TASK"cEND" NEW -> id %d\n", Record->LObjId);
    evtimer_set(&task->time_ev, scheduleICMPTask, task);
    evtimer_add(&task->time_ev, &tv);
    memcpy(&((task)->Record), Record, sizeof (*Record));



};


void initTesterVar() {
    IcmpPool.fd = 0;
    LIST_INIT(&IcmpReadHead);
    LIST_INIT(&IcmpWriteHead);
}


int main(int arg, char **argv) {
    event_init();
    initTesterVar();
    #define COUNT 500

    struct _Tester_Cfg_Record RequestICMP[COUNT];
    int i = 0;
    u32 ip = 0;
    inet_aton("213.248.62.7", &ip);

    for (i = 0; i < COUNT; i++) {
        bzero(&RequestICMP[i], sizeof (struct _Tester_Cfg_Record));
        RequestICMP[i].IP = ip;
        RequestICMP[i].LObjId = i;
    }

    for (i = 0; i < COUNT; i++) {
        addTask(&RequestICMP[i]);
    }


    event_dispatch();
    printf(cGREEN"\nICMP STAT"cEND" OK:%d , TIMEOUT: %d\n\n", icmpOK, icmpTIMEOUT);
    return 0;
}


