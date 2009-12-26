#include "include.h"

struct timeval tv;
static struct event_base *base;
struct DNSTask *dnstask;

static inline char * getPTR(char *host) {
    char arpa[100];
    u_char *ip;
    in_addr_t in = inet_addr((const char *) host);
    if (in == -1) return host;
    ip = (char *) & in;
    snprintf(arpa, 100, "%d.%d.%d.%d.in-addr.arpa", ip[3], ip[2], ip[1], ip[0]);
    return arpa;
}

inline static const unsigned char *CheckPatternAfterParseAnswer(struct DNSTask *dnstask, const unsigned char *aptr, const unsigned char *abuf, int alen) {

    struct Task *task = dnstask->task;
    const unsigned char *p;
    int type, dnsclass, ttl, dlen, status;
    long len;
    char addr[46];

    /*
    if (task->LObjId == 1148) raise(SIGTRAP);
     */

    union {
        unsigned char * as_uchar;
        char * as_char;
    } name;

    status = ares_expand_name(aptr, abuf, alen, &name.as_char, &len);
    if (status != ARES_SUCCESS)
        return NULL;
    aptr += len;

    if (aptr + RRFIXEDSZ > abuf + alen) {
        ares_free_string(name.as_char);
        return NULL;
    }

    type = DNS_RR_TYPE(aptr);
    dnsclass = DNS_RR_CLASS(aptr);
    ttl = DNS_RR_TTL(aptr);
    dlen = DNS_RR_LEN(aptr);
    aptr += RRFIXEDSZ;
    if (aptr + dlen > abuf + alen) {
        ares_free_string(name.as_char);
        return NULL;
    }
    ares_free_string(name.as_char);

    switch (type) {
        case T_CNAME:
        case T_MB:
        case T_MD:
        case T_MF:
        case T_MG:
        case T_MR:
        case T_NS:
        case T_PTR:

            status = ares_expand_name(aptr, abuf, alen, &name.as_char, &len);
            if (status != ARES_SUCCESS) {
                debug("error T_%s compare, %s andd ttl %d error - %d", type_name(type), dnstask->taskPattern, dnstask->taskTTL, status);
                return NULL;
            }
            debug("T_%s compare, %s:%08x on %s and ttl %d on %d", type_name(type), dnstask->taskPattern, dnstask->taskPattern, name.as_char, ttl, dnstask->taskTTL);
            if ((dnstask->taskPattern[0] == 0 or !memcmp(name.as_char, dnstask->taskPattern, dnstask->taskPatternLen)) and(dnstask->taskTTL == 0 or ttl == dnstask->taskTTL)) {
                dnstask->task->code = STATE_DONE;
            }
            ares_free_string(name.as_char);
            break;

        case T_HINFO:
            /* The RR data is two length-counted character strings. */
            p = aptr;
            len = *p;
            if (p + len + 1 > aptr + dlen)
                return NULL;
            debug("\t%.*s", (int) len, p + 1);
            p += len + 1;
            len = *p;
            if (p + len + 1 > aptr + dlen)
                return NULL;
            debug("\t%.*s", (int) len, p + 1);
            break;

        case T_MINFO:
            /* The RR data is two domain names. */
            p = aptr;
            status = ares_expand_name(p, abuf, alen, &name.as_char, &len);
            if (status != ARES_SUCCESS)
                return NULL;
            debug("\t%s.", name.as_char);
            ares_free_string(name.as_char);
            p += len;
            status = ares_expand_name(p, abuf, alen, &name.as_char, &len);
            if (status != ARES_SUCCESS)
                return NULL;
            debug("\t%s.", name.as_char);
            ares_free_string(name.as_char);
            break;

        case T_MX:
            if (dlen < 2) {
                return NULL;
            }
            status = ares_expand_name(aptr + 2, abuf, alen, &name.as_char, &len);
            if (status != ARES_SUCCESS) {
                return NULL;
            }

            debug("T_MX compare %s on %s and ttl %d on %d\n", dnstask->taskPattern, name.as_char, ttl, dnstask->taskTTL);
            if ((dnstask->taskPattern[0] = 0 or !memcmp(name.as_char, dnstask->taskPattern, dnstask->taskPatternLen)) and(dnstask->taskTTL == 0 or ttl == dnstask->taskTTL)) {
                dnstask->task->code = STATE_DONE;
            }
            ares_free_string(name.as_char);


            break;

        case T_SOA:
            /* The RR data is two domain names and then five four-byte
             * numbers giving the serial number and some timeouts.
             */
            p = aptr;
            status = ares_expand_name(p, abuf, alen, &name.as_char, &len);
            if (status != ARES_SUCCESS)
                return NULL;
            debug("\t%s.", name.as_char);
            ares_free_string(name.as_char);
            p += len;
            status = ares_expand_name(p, abuf, alen, &name.as_char, &len);
            if (status != ARES_SUCCESS)
                return NULL;
            debug("\t\t\t\t\t\t%s.", name.as_char);
            ares_free_string(name.as_char);
            p += len;
            if (p + 20 > aptr + dlen)
                return NULL;
            debug("\t\t\t\t\t\t( %lu %lu %lu %lu %lu )",
                    (unsigned long) DNS__32BIT(p), (unsigned long) DNS__32BIT(p + 4),
                    (unsigned long) DNS__32BIT(p + 8), (unsigned long) DNS__32BIT(p + 12),
                    (unsigned long) DNS__32BIT(p + 16));
            break;

        case T_TXT:
            /* The RR data is one or more length-counted character
             * strings. */
            p = aptr;
            while (p < aptr + dlen) {
                len = *p;
                if (p + len + 1 > aptr + dlen)
                    return NULL;

                //printf("\t%.*s", (int) len, p + 1);
                debug("T_TXT compare %s on %s and ttl %d on %d", dnstask->taskPattern, p + 1, ttl, dnstask->taskTTL);
                if (!memcmp(p + 1, dnstask->taskPattern, dnstask->taskPatternLen) and ttl == dnstask->taskTTL) {
                    dnstask->task->code = STATE_DONE;
                }

                p += len + 1;
            }
            break;

        case T_A:
            /* The RR data is a four-byte Internet address. */
            inet_ntop(AF_INET, aptr, addr, sizeof (addr));
            debug("T_A compare %s on %s (size %d) and ttl %d on %d", dnstask->taskPattern, addr, dnstask->taskPatternLen, ttl, dnstask->taskTTL);
            /*
                        if (dnstask->task->LObjId == 1056) raise(SIGSEGV);
             */

            if (dlen == 4 and(dnstask->taskPattern[0] = 0 or !memcmp(addr, dnstask->taskPattern, dnstask->taskPatternLen)) and(dnstask->taskTTL == 0 or ttl == dnstask->taskTTL)) {
                dnstask->task->code = STATE_DONE;
            }
            break;

        case T_AAAA:
            /* The RR data is a 16-byte IPv6 address. */
            if (dlen != 16)
                return NULL;
            debug("\t%s", inet_ntop(AF_INET6, aptr, addr, sizeof (addr)));
            break;

        case T_WKS:
            /* Not implemented yet */
            break;

        case T_SRV:
            /* The RR data is three two-byte numbers representing the
             * priority, weight, and port, followed by a domain name.
             */

            printf("\t%d", DNS__16BIT(aptr));
            printf(" %d", DNS__16BIT(aptr + 2));
            printf(" %d", DNS__16BIT(aptr + 4));

            status = ares_expand_name(aptr + 6, abuf, alen, &name.as_char, &len);
            if (status != ARES_SUCCESS)
                return NULL;
            printf("\t%s.", name.as_char);
            ares_free_string(name.as_char);
            break;

        case T_NAPTR:

            printf("\t%d", DNS__16BIT(aptr)); /* order */
            printf(" %d\n", DNS__16BIT(aptr + 2)); /* preference */

            p = aptr + 4;
            status = ares_expand_string(p, abuf, alen, &name.as_uchar, &len);
            if (status != ARES_SUCCESS)
                return NULL;
            printf("\t\t\t\t\t\t%s\n", name.as_char);
            ares_free_string(name.as_char);
            p += len;

            status = ares_expand_string(p, abuf, alen, &name.as_uchar, &len);
            if (status != ARES_SUCCESS)
                return NULL;
            printf("\t\t\t\t\t\t%s\n", name.as_char);
            ares_free_string(name.as_char);
            p += len;

            status = ares_expand_string(p, abuf, alen, &name.as_uchar, &len);
            if (status != ARES_SUCCESS)
                return NULL;
            printf("\t\t\t\t\t\t%s\n", name.as_char);
            ares_free_string(name.as_char);
            p += len;

            status = ares_expand_string(p, abuf, alen, &name.as_uchar, &len);
            if (status != ARES_SUCCESS)
                return NULL;
            printf("\t\t\t\t\t\t%s", name.as_char);
            ares_free_string(name.as_char);
            break;


        default:
            printf("\t[Unknown RR; cannot parse]");
            break;
    }
    if (dnstask->task->code != STATE_DONE) {
        return aptr + dlen;
    } else {
        return NULL;
    }
}

void OnEventDNSTask(int fd, short event, void *arg) {
    struct DNSTask *dnstask = (struct DNSTask *) arg;
    debug("event:%s, LobjId:%d, Hostname %s", getActionText(event), dnstask->task->LObjId, dnstask->task->Record.HostName);
    if (event & EV_READ) {
        ares_process_fd(dnstask->channel, fd, ARES_SOCKET_BAD);
    } else if (event & EV_TIMEOUT) {
        dnstask->task->code = STATE_TIMEOUT;
        dnstask->task->callback(dnstask->task);
    }

    if (!dnstask->isNeed) {
        ares_destroy(dnstask->channel);
        event_del(&dnstask->ev);
    }
}

void DNSTaskResolvCallback(void *arg, int status, int timeouts, unsigned char *abuf, int alen) {
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

    dnstask->task->code = STATE_ERROR;

    switch (dnstask->role) {
        case DNS_TASK:
            for (i = 0; i < ancount; i++) {
                debug("try %d %d ", dnstask->task->LObjId, ancount);
                aptr = CheckPatternAfterParseAnswer(dnstask, aptr, abuf, alen);
                if (aptr == NULL) break;
            }
            dnstask->task->callback(dnstask->task);


            break;

    }


}

void makeDNSTask(struct DNSTask * dnstask) {
    int type;
    struct ares_options options;
    int optmask;


    switch (dnstask->role) {
        case DNS_TASK:
            gettimeofday(&dnstask->CheckDt, NULL);
            optmask = ARES_OPT_FLAGS;
            options.servers = getNulledMemory(sizeof (struct in_addr));
            options.nservers = 1;
            options.flags = ARES_FLAG_NOCHECKRESP;
            optmask |= ARES_OPT_SERVERS;
            memcpy(options.servers, &dnstask->task->resolv->NSIP, sizeof (struct in_addr));
            ares_init_options(&dnstask->channel, &options, optmask);
            MSToTimeval(dnstask->task->Record.TimeOut, tv);
            type = dnstask->taskType;
            break;
        default:
            return;
            break;
    }


    ares_query(dnstask->channel, getPTR(dnstask->task->Record.HostName), C_IN, type, DNSTaskResolvCallback, dnstask);
    ares_getsock(dnstask->channel, &dnstask->fd, 1);
    event_assign(&dnstask->ev, base, dnstask->fd, EV_READ | EV_TIMEOUT, OnEventDNSTask, dnstask);
    event_add(&dnstask->ev, &tv);
}

void timerDNSTask(int fd, short action, void *arg) {

    struct Task *task = arg;
    struct DNSTask *dnstask = (struct DNSTask *) task->poll;

    /*
        if (task->LObjId == 1151) raise(SIGTRAP);
     */

    if (task->isEnd) {
        evtimer_del(&task->time_ev);
        deleteTask(task);
        return;
    }
    setNextTimer(task);

    if (task->resolv and task->resolv->NSIP) {
        debug("LobjId:%d, Role %s, Hostname %s", task->LObjId, getRoleText(dnstask->role), task->Record.HostName);
        makeDNSTask(dnstask);
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

void initDNSTester() {
    pthread_t threads;
    pthread_create(&threads, NULL, (void*) initThread, NULL);
}

struct event_base *getDNSBase() {
    return base;
}
