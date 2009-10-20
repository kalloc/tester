#include "include.h"
static struct event_base *base;

struct IcmpPooler {
    int write_fd;
    int read_fd;
    struct event read_ev;
    struct event write_ev;
} IcmpPoll;

LIST_HEAD(IcmpListHead, IcmpListEntry) IcmpWriteHead, IcmpReadHead;
struct IcmpListHead *IcmpHeadPtr; /* List head. */

struct IcmpListEntry {
    struct Task *task;
    LIST_ENTRY(IcmpListEntry) entries; /* List. */
} *IcmpEntryPtr;

struct timeval tv;

void OnReadICMPTask(int fd, short event, void *arg) {

    if (LIST_EMPTY(&IcmpReadHead) and IcmpPoll.read_fd != 0) {
        event_del(&IcmpPoll.read_ev);
        close(IcmpPoll.read_fd);
        IcmpPoll.read_fd = 0;
        return;
    }

    struct sockaddr_in sa;
    struct iphdr *iphdr;
    struct icmp *ping;
    struct stICMPInfo *icmp_poll;
    struct Task *task;
    char packet[100];
    short len = 100;

    u8 state, countPerIter = 0;
    u16 IcmpId, lObjId;

    socklen_t salen = sizeof (struct sockaddr);
    bzero(&sa, salen);

    gettimeofday(&tv, NULL);


    while (++countPerIter < 200 && (len = recvfrom(IcmpPoll.read_fd, (char *) & packet, len, 0, (struct sockaddr *) & sa, (socklen_t *) & salen)) > 0) {

        if (len < 32) continue;

        iphdr = (struct iphdr *) packet;

        if (iphdr->protocol != 1) continue;
        ping = (struct icmp *) (iphdr + 1);

        if (ping->icmp_type == ICMP_ECHOREPLY) {
            lObjId = *(u32 *) (packet + 28);
            IcmpId = ping->icmp_id;
            state = STATE_DONE;
        } else if (ping->icmp_type == ICMP_DEST_UNREACH || ping->icmp_type == ICMP_HOST_UNKNOWN || ping->icmp_type == ICMP_TIME_EXCEEDED || ping->icmp_type == ICMP_SOURCE_QUENCH) {
            lObjId = *(u32 *) (packet + 56);
            IcmpId = *(u16 *) (packet + 52);
            state = STATE_ERROR;
        } else {
            continue;
        }

        LIST_FOREACH(IcmpEntryPtr, &IcmpReadHead, entries) {
            task = IcmpEntryPtr->task;
            icmp_poll = (struct stICMPInfo *) IcmpEntryPtr->task->poll;
            if (lObjId != task->LObjId || IcmpId != icmp_poll->id) continue;

            icmp_poll->ProblemICMP_Code = ping->icmp_code;
            icmp_poll->ProblemICMP_Type = ping->icmp_type;
            icmp_poll->ProblemIP = (u32) (state == STATE_DONE ? sa.sin_addr.s_addr : 0);

            task->code = state;

            debug("id %d, Module %s for %s [%s:%d] [Icmp_Code: %d, Icmp_Type: %d, ms: %d]", IcmpEntryPtr->task->Record.LObjId, getModuleText(IcmpEntryPtr->task->Record.ModType), IcmpEntryPtr->task->Record.HostName, ipString(IcmpEntryPtr->task->Record.IP), IcmpEntryPtr->task->Record.Port, icmp_poll->ProblemICMP_Code, icmp_poll->ProblemICMP_Type, icmp_poll->DelayMS);
            if (task->callback) {
                task->callback(task);
            }

            LIST_REMOVE(IcmpEntryPtr, entries);
            free(IcmpEntryPtr);
            if (task->isEnd) {
                OnDisposeICMPTask(task);
            }
            break;
        }
    }

    if (LIST_EMPTY(&IcmpReadHead)) {
        event_del(&IcmpPoll.read_ev);
        close(IcmpPoll.read_fd);
        IcmpPoll.read_fd = 0;
    }

}

void OnWriteICMPTask(int fd, short action, void *arg) {
    struct icmp *ping;
    struct sockaddr_in sa;
    char packet[12];

    gettimeofday(&tv, NULL);

    if (IcmpPoll.read_fd == 0) {
        IcmpPoll.read_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        evutil_make_socket_nonblocking(IcmpPoll.read_fd);
        event_set(&IcmpPoll.read_ev, IcmpPoll.read_fd, EV_READ | EV_PERSIST, OnReadICMPTask, NULL);
        event_add(&IcmpPoll.read_ev, NULL);
    }

    LIST_FOREACH(IcmpEntryPtr, &IcmpWriteHead, entries) {
        debug("id %d, Module %s for %s [%s:%d]", IcmpEntryPtr->task->Record.LObjId, getModuleText(IcmpEntryPtr->task->Record.ModType), IcmpEntryPtr->task->Record.HostName, ipString(IcmpEntryPtr->task->Record.IP), IcmpEntryPtr->task->Record.Port);

        bzero(&packet, 12);
        *(u32 *) (&packet[8]) = IcmpEntryPtr->task->LObjId;
        ping = (struct icmp *) & packet;
        ping->icmp_type = ICMP_ECHO;
        ping->icmp_code = 0;
        ping->icmp_seq = 8383;
        ((struct stICMPInfo *) (IcmpEntryPtr->task->poll))->id = ping->icmp_id = rand();
        ping->icmp_cksum = in_cksum((u_short *) & packet, 12);
        /*
                ((struct stICMPInfo *) (IcmpEntryPtr->task->poll))->ProblemIP = 0;
         */
        sa.sin_addr.s_addr = (in_addr_t) IcmpEntryPtr->task->Record.IP;
        sa.sin_family = AF_INET;
        sa.sin_port = htons(0);
        bzero(&sa.sin_zero, 8);
        sendto(IcmpPoll.write_fd, (char *) packet, 12, 0, (struct sockaddr *) & sa, sizeof (struct sockaddr));

        gettimeofday(&((struct stICMPInfo *) IcmpEntryPtr->task->poll)->CheckDt, NULL);

        LIST_REMOVE(IcmpEntryPtr, entries);
        LIST_INSERT_HEAD(&IcmpReadHead, IcmpEntryPtr, entries);
        break;
    }
    if (LIST_EMPTY(&IcmpWriteHead)) {
        event_del(&IcmpPoll.write_ev);
        close(IcmpPoll.write_fd);
        IcmpPoll.write_fd = 0;
    }
}

void timerTimeoutICMPTask(int fd, short action, void *arg) {
    struct Task *task = (struct Task *) arg;

    debug("id %d, Module %s for %s [%s:%d]", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);
    //если состояние подключения не изменилось, то значит что какого либо ответа не было получено
    task->code = STATE_TIMEOUT;

    LIST_FOREACH(IcmpEntryPtr, &IcmpReadHead, entries) {
        if (task->LObjId != IcmpEntryPtr->task->LObjId) continue;
        LIST_REMOVE(IcmpEntryPtr, entries);
        free(IcmpEntryPtr);
        break;
    }

    LIST_FOREACH(IcmpEntryPtr, &IcmpWriteHead, entries) {
        if (task->LObjId != IcmpEntryPtr->task->LObjId) continue;
        LIST_REMOVE(IcmpEntryPtr, entries);
        free(IcmpEntryPtr);
        break;
    }


    //добавляем репорт о ошибке
    if (task->callback) {
        task->callback(task);
    }

    if (task->isEnd == TRUE) {
        OnDisposeICMPTask(task);
    } else {
        evtimer_del(&task->read);
    }

}

void timerICMPTask(int fd, short action, void *arg) {
    struct Task *task = (struct Task *) arg;

    debug("id %d, Module %s for %s [%s:%d]", task->Record.LObjId, getModuleText(task->Record.ModType), task->Record.HostName, ipString(task->Record.IP), task->Record.Port);
    if (!task->Record.IP) return;

    if (task->isEnd == TRUE) {
        OnDisposeICMPTask(task);
        return;
    }

    if (IcmpPoll.write_fd == 0) {
        IcmpPoll.write_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        evutil_make_socket_nonblocking(IcmpPoll.write_fd);
        event_set(&IcmpPoll.write_ev, IcmpPoll.write_fd, EV_WRITE | EV_PERSIST, OnWriteICMPTask, NULL);
        event_add(&IcmpPoll.write_ev, NULL);
    }


    IcmpEntryPtr = getNulledMemory(sizeof (struct IcmpListEntry)); /* Insert at the head. */
    IcmpEntryPtr->task = task;
    LIST_INSERT_HEAD(&IcmpWriteHead, IcmpEntryPtr, entries);
    task->code = STATE_TIMEOUT;


    setNextTimer(task);

    if (task->Record.TimeOut > 1000) {
        tv.tv_sec = task->Record.TimeOut / 1000 / 2;
        tv.tv_usec = (task->Record.TimeOut % 1000)*1000;
    } else {
        tv.tv_usec = task->Record.TimeOut * 1000 / 2;
        tv.tv_sec = 0;
    }


    event_assign(&task->read, base, -1, 0, timerTimeoutICMPTask, task);
    evtimer_add(&task->read, &tv);
}

void OnDisposeICMPTask(struct Task * task) {
    debug("%d - %s %d", task->LObjId, ipString(task->Record.IP), STATE_CONNECTED);
    evtimer_del(&task->time_ev);
    evtimer_del(&task->read);
    deleteTask(task);
};

static void timerMain(int fd, short action, void *arg) {
    struct event *ev = (struct event *) arg;
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    evtimer_add(ev, &tv);

}

static void initICMPThread() {
    base = event_base_new();

    struct event ev;

    tv.tv_sec = 60;
    tv.tv_usec = 0;
    event_assign(&ev, base, -1, 0, timerMain, &ev);
    evtimer_add(&ev, &tv);
    event_base_dispatch(base);
}

void initICMPTester() {
    pthread_t threads;
    IcmpPoll.write_fd = IcmpPoll.read_fd = 0;
    LIST_INIT(&IcmpReadHead);
    LIST_INIT(&IcmpWriteHead);
    pthread_create(&threads, NULL, (void*) initICMPThread, NULL);
}


struct event_base *getICMPBase() {
    return base;
}


