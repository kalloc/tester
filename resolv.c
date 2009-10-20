#include "include.h"

struct timeval tv;
static struct event_base *base;
struct DNSTask *dnstask;

void OnEventResolv(int fd, short event, void *arg) {
    struct DNSTask *dnstask = (struct DNSTask *) arg;
    debug("event:%s, LobjId:%d, Hostname %s", getActionText(event), dnstask->task->LObjId, dnstask->task->Record.HostName);
    if (event & EV_READ) {
        ares_process_fd(dnstask->channel, fd, ARES_SOCKET_BAD);
    }

    if (!dnstask->isNeed) {
        ares_destroy(dnstask->channel);
        event_del(&dnstask->ev);
    }
}

void ResolvCallback(void *arg, int status, int timeouts, unsigned char *abuf, int alen) {
    struct DNSTask *dnstask = (struct DNSTask *) arg;
    char *query;
    int id, qr, opcode, aa, tc, rd, ra, rcode, len;
    u32 ip;
    unsigned int qdcount, ancount, nscount, arcount, i;
    const unsigned char *aptr;
    debug("LobjId:%d, Hostname %s", dnstask->task->LObjId, dnstask->task->Record.HostName);
    if (status != ARES_SUCCESS and status != ARES_ENODATA) {
        return;
    }

    /* Parse the answer header. */
    id = DNS_HEADER_QID(abuf);
    qr = DNS_HEADER_QR(abuf);
    opcode = DNS_HEADER_OPCODE(abuf);
    aa = DNS_HEADER_AA(abuf);
    tc = DNS_HEADER_TC(abuf);
    rd = DNS_HEADER_RD(abuf);
    ra = DNS_HEADER_RA(abuf);
    rcode = DNS_HEADER_RCODE(abuf);
    qdcount = DNS_HEADER_QDCOUNT(abuf);
    ancount = DNS_HEADER_ANCOUNT(abuf);
    nscount = DNS_HEADER_NSCOUNT(abuf);
    arcount = DNS_HEADER_ARCOUNT(abuf);

    aptr = abuf + HFIXEDSZ;

    ares_expand_name(aptr, abuf, alen, &query, &len);

    for (i = 0; i < qdcount; i++) {
        aptr = skip_question(aptr, abuf, alen);
        if (aptr == NULL) return;
    }
    struct hostent *he;
    struct addrttl addrttls;
    char *ptr2 = NULL, *ptr = NULL;
    switch (dnstask->role) {
        case DNS_RESOLV:
            he = getNulledMemory(sizeof (*he));
            ares_parse_a_reply(abuf, alen, &he, &addrttls, &i);

            if (he->h_addrtype) {
                if (he->h_addr) {
                    ip = *(u32 *) he->h_addr_list[id % i];

                    debug("FOUND IP %s -> %s", ipString(ip), he->h_name);
                    if (ip) {
                        dnstask->task->Record.IP = ip;
                    }

                    ares_free_hostent(he);
                }
            } else {
                free(he);
            }

            break;
        case DNS_GETNS:

            he = getNulledMemory(sizeof (*he));
            switch (getDNSTypeAfterParseAnswer(aptr, abuf, alen)) {
                case T_A:
                    ares_parse_a_reply(abuf, alen, &he, &addrttls, &i);
                    break;
                case T_NS:
                    ares_parse_ns_reply(abuf, alen, &he);
                    break;
                default:
                    tv.tv_usec = 0;
                    tv.tv_sec = config.timeout;
                    ptr = query;
                    i = 0;
                    while (*ptr++ != 0) {
                        if (ptr[0] == '.') {
                            if (!ptr2)
                                ptr2 = ptr + 1;
                            i++;
                        }
                    }
                    if (i <= 1) {
                        dnstask->task->code = STATE_ERROR;
                        dnstask->task->callback(dnstask->task);
                        break;
                    }
                    dnstask->isNeed = 1;
                    ares_query(dnstask->channel, ptr2, C_IN, T_NS, ResolvCallback, dnstask);
                    ares_getsock(dnstask->channel, &dnstask->fd, 1);
                    event_assign(&dnstask->ev, base, dnstask->fd, EV_READ | EV_TIMEOUT, OnEventResolv, dnstask);
                    event_add(&dnstask->ev, &tv);
                    break;
            }
            if (he->h_addrtype) {
                if (he->h_addr) {
                    ip = *(u32 *) he->h_addr_list[0];
                    dnstask->isNeed = 0;
                    debug("FOUND IP %s -> %s", ipString(ip), he->h_name);
                    if (ip) {
                        dnstask->NSIP = ip;
                    }
                } else {
                    dnstask->isNeed = 1;
                    for (i = 0; he->h_aliases[i] != 0; i++);
                    i = id % i;
                    debug("FOUND NEW HOST %s -> %s", he->h_aliases[i], he->h_name);

                    tv.tv_usec = 0;
                    tv.tv_sec = config.timeout;
                    ares_query(dnstask->channel, he->h_aliases[i], C_IN, T_A, ResolvCallback, dnstask);
                    ares_getsock(dnstask->channel, &dnstask->fd, 1);
                    event_assign(&dnstask->ev, base, dnstask->fd, EV_READ | EV_TIMEOUT, OnEventResolv, dnstask);
                    event_add(&dnstask->ev, &tv);
                }
                ares_free_hostent(he);
            } else {
                free(he);
            }
            break;

    }


}

void makeResolv(struct DNSTask * dnstask) {
    int type;
    struct ares_options options;
    int optmask;


    switch (dnstask->role) {
        case DNS_RESOLV:
            type = T_A;
            ares_init(&dnstask->channel);
            tv.tv_usec = 0;
            tv.tv_sec = config.timeout;
            break;
        case DNS_GETNS:
            type = T_NS;
            ares_init(&dnstask->channel);
            tv.tv_usec = 0;
            tv.tv_sec = config.timeout;
            break;
        default:
            return;
            break;
    }


    ares_query(dnstask->channel, getPTR(dnstask->task->Record.HostName), C_IN, type, ResolvCallback, dnstask);
    ares_getsock(dnstask->channel, &dnstask->fd, 1);
    event_assign(&dnstask->ev, base, dnstask->fd, EV_READ | EV_TIMEOUT, OnEventResolv, dnstask);
    event_add(&dnstask->ev, &tv);
}

void timerResolv(int fd, short action, void *arg) {

    struct Task *task = (struct Task *) arg;
    if (!task->resolv) return;
    if (task->resolv->isNeed == 0) makeResolv(task->resolv);
    debug("LobjId:%d, Role %s, Hostname %s", task->LObjId, getRoleText(task->resolv->role), task->Record.HostName);

    if (task->Record.ResolvePeriod) {
        tv.tv_sec = task->Record.ResolvePeriod;
        tv.tv_usec = 0;
        event_assign(&task->resolv->timer, base, -1, 0, timerResolv, task);
        evtimer_add(&task->resolv->timer, &tv);
    }
}


static void timerMain(int fd, short action, void *arg) {
    struct event *ev = (struct event *) arg;
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    evtimer_add(ev, &tv);

}

static void initThread() {
    base = event_base_new();

    struct event ev;

    tv.tv_sec = 60;
    tv.tv_usec = 0;
    event_assign(&ev, base, -1, 0, timerMain, &ev);
    evtimer_add(&ev, &tv);
    event_base_dispatch(base);
}

void initResolv() {
    pthread_t threads;
    pthread_create(&threads, NULL, (void*) initThread, NULL);
}

struct event_base *getResolvBase() {
    return base;
}


