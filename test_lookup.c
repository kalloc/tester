#include "include.h"

struct timeval tv;
static struct event_base *base;

struct nv {
    const char *name;
    int value;
};
static const struct nv classes[] = {
    { "IN", C_IN},
    { "CHAOS", C_CHAOS},
    { "HS", C_HS},
    { "ANY", C_ANY}
};
static const int nclasses = sizeof (classes) / sizeof (classes[0]);
static const struct nv types[] = {
    { "A", T_A},
    { "NS", T_NS},
    { "MD", T_MD},
    { "MF", T_MF},
    { "CNAME", T_CNAME},
    { "SOA", T_SOA},
    { "MB", T_MB},
    { "MG", T_MG},
    { "MR", T_MR},
    { "NULL", T_NULL},
    { "WKS", T_WKS},
    { "PTR", T_PTR},
    { "HINFO", T_HINFO},
    { "MINFO", T_MINFO},
    { "MX", T_MX},
    { "TXT", T_TXT},
    { "RP", T_RP},
    { "AFSDB", T_AFSDB},
    { "X25", T_X25},
    { "ISDN", T_ISDN},
    { "RT", T_RT},
    { "NSAP", T_NSAP},
    { "NSAP_PTR", T_NSAP_PTR},
    { "SIG", T_SIG},
    { "KEY", T_KEY},
    { "PX", T_PX},
    { "GPOS", T_GPOS},
    { "AAAA", T_AAAA},
    { "LOC", T_LOC},
    { "SRV", T_SRV},
    { "AXFR", T_AXFR},
    { "MAILB", T_MAILB},
    { "MAILA", T_MAILA},
    { "NAPTR", T_NAPTR},
    { "ANY", T_ANY}
};
static const int ntypes = sizeof (types) / sizeof (types[0]);
/*
static const char *opcodes[] = {
    "QUERY", "IQUERY", "STATUS", "(reserved)", "NOTIFY",
    "(unknown)", "(unknown)", "(unknown)", "(unknown)",
    "UPDATEA", "UPDATED", "UPDATEDA", "UPDATEM", "UPDATEMA",
    "ZONEINIT", "ZONEREF"
};
 */
struct in_addr inaddr;

/*
static const char *rcodes[] = {
    "NOERROR", "FORMERR", "SERVFAIL", "NXDOMAIN", "NOTIMP", "REFUSED",
    "(unknown)", "(unknown)", "(unknown)", "(unknown)", "(unknown)",
    "(unknown)", "(unknown)", "(unknown)", "(unknown)", "NOCHANGE"
};
 */

static const char *type_name(int type) {
    int i;

    for (i = 0; i < ntypes; i++) {
        if (types[i].value == type)
            return types[i].name;
    }
    return "(unknown)";
}

static const char *class_name(int dnsclass) {
    int i;

    for (i = 0; i < nclasses; i++) {
        if (classes[i].value == dnsclass)
            return classes[i].name;
    }
    return "(unknown)";
}

inline static const unsigned char *skip_question(const unsigned char *aptr, const unsigned char *abuf, int alen) {
    char *name;
    int status;
    long len;
    status = ares_expand_name(aptr, abuf, alen, &name, &len);
    if (status != ARES_SUCCESS) return NULL;
    aptr += len;
    if (aptr + QFIXEDSZ > abuf + alen) {
        ares_free_string(name);
        return NULL;
    }
    aptr += QFIXEDSZ;
    ares_free_string(name);
    return aptr;
}

inline static const unsigned char *CheckPatternAfterParseAnswer(const unsigned char *aptr, const unsigned char *abuf, int alen) {
    const unsigned char *p;
    int type, dnsclass, ttl, dlen, status;
    long len;
    char addr[46];

    union {
        unsigned char * as_uchar;
        char * as_char;
    } name;

    /* Parse the RR name. */
    status = ares_expand_name(aptr, abuf, alen, &name.as_char, &len);
    if (status != ARES_SUCCESS)
        return NULL;
    aptr += len;

    /* Make sure there is enough data after the RR name for the fixed
     * part of the RR.
     */
    if (aptr + RRFIXEDSZ > abuf + alen) {
        ares_free_string(name.as_char);
        return NULL;
    }

    /* Parse the fixed part of the RR, and advance to the RR data
     * field. */
    type = DNS_RR_TYPE(aptr);
    dnsclass = DNS_RR_CLASS(aptr);
    ttl = DNS_RR_TTL(aptr);
    dlen = DNS_RR_LEN(aptr);
    aptr += RRFIXEDSZ;
    if (aptr + dlen > abuf + alen) {
        ares_free_string(name.as_char);
        return NULL;
    }

    /* Display the RR name, class, and type. */
    printf("\t%-15s.\t%d", name.as_char, ttl);
    if (dnsclass != C_IN)
        printf("\t%s", class_name(dnsclass));
    printf("\t%s", type_name(type));
    ares_free_string(name.as_char);

    /* Display the RR data.  Don't touch aptr. */
    switch (type) {
        case T_CNAME:
        case T_MB:
        case T_MD:
        case T_MF:
        case T_MG:
        case T_MR:
        case T_NS:
        case T_PTR:
            /* For these types, the RR data is just a domain name. */
            status = ares_expand_name(aptr, abuf, alen, &name.as_char, &len);
            if (status != ARES_SUCCESS)
                return NULL;
            printf("\t%s.", name.as_char);
            ares_free_string(name.as_char);
            break;

        case T_HINFO:
            /* The RR data is two length-counted character strings. */
            p = aptr;
            len = *p;
            if (p + len + 1 > aptr + dlen)
                return NULL;
            printf("\t%.*s", (int) len, p + 1);
            p += len + 1;
            len = *p;
            if (p + len + 1 > aptr + dlen)
                return NULL;
            printf("\t%.*s", (int) len, p + 1);
            break;

        case T_MINFO:
            /* The RR data is two domain names. */
            p = aptr;
            status = ares_expand_name(p, abuf, alen, &name.as_char, &len);
            if (status != ARES_SUCCESS)
                return NULL;
            printf("\t%s.", name.as_char);
            ares_free_string(name.as_char);
            p += len;
            status = ares_expand_name(p, abuf, alen, &name.as_char, &len);
            if (status != ARES_SUCCESS)
                return NULL;
            printf("\t%s.", name.as_char);
            ares_free_string(name.as_char);
            break;

        case T_MX:
            /* The RR data is two bytes giving a preference ordering, and
             * then a domain name.
             */
            if (dlen < 2)
                return NULL;
            printf("\t%d", DNS__16BIT(aptr));
            status = ares_expand_name(aptr + 2, abuf, alen, &name.as_char, &len);
            if (status != ARES_SUCCESS)
                return NULL;
            printf("\t%s.", name.as_char);
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
            printf("\t%s.\n", name.as_char);
            ares_free_string(name.as_char);
            p += len;
            status = ares_expand_name(p, abuf, alen, &name.as_char, &len);
            if (status != ARES_SUCCESS)
                return NULL;
            printf("\t\t\t\t\t\t%s.\n", name.as_char);
            ares_free_string(name.as_char);
            p += len;
            if (p + 20 > aptr + dlen)
                return NULL;
            printf("\t\t\t\t\t\t( %lu %lu %lu %lu %lu )",
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
                printf("\t%.*s", (int) len, p + 1);
                p += len + 1;
            }
            break;

        case T_A:
            /* The RR data is a four-byte Internet address. */
            if (dlen != 4)
                return NULL;
            printf("\t%s", inet_ntop(AF_INET, aptr, addr, sizeof (addr)));
            break;

        case T_AAAA:
            /* The RR data is a 16-byte IPv6 address. */
            if (dlen != 16)
                return NULL;
            printf("\t%s", inet_ntop(AF_INET6, aptr, addr, sizeof (addr)));
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
    printf("\n");

    return aptr + dlen;
}

inline static const u32 ResolvAfterParseAnswer(const unsigned char *aptr, const unsigned char *abuf, int alen) {
    int type, dnsclass, ttl, dlen, status;
    long len;

    union {
        unsigned char * as_uchar;
        char * as_char;
    } name;

    /* Parse the RR name. */
    status = ares_expand_name(aptr, abuf, alen, &name.as_char, &len);
    if (status != ARES_SUCCESS)
        return 0;
    aptr += len;

    /* Make sure there is enough data after the RR name for the fixed
     * part of the RR.
     */
    if (aptr + RRFIXEDSZ > abuf + alen) {
        ares_free_string(name.as_char);
        return 0;
    }

    /* Parse the fixed part of the RR, and advance to the RR data
     * field. */
    type = DNS_RR_TYPE(aptr);
    dnsclass = DNS_RR_CLASS(aptr);
    ttl = DNS_RR_TTL(aptr);
    dlen = DNS_RR_LEN(aptr);
    aptr += RRFIXEDSZ;
    if (aptr + dlen > abuf + alen) {
        ares_free_string(name.as_char);
        return 0;
    }

    ares_free_string(name.as_char);

    if (type != T_A) return 0;

    if (dlen != 4)
        return 0;
    return *(const u32 *) aptr;
    /*
        printf("\t%s\n", inet_ntop(AF_INET, aptr, addr, sizeof (addr)));
        hexPrint(addr, sizeof (addr));
        return addr;
     */
}




struct timeval tv;
char buf[BUFLEN];

void OnEventDNSTask(int fd, short event, void *arg) {
    struct Task *task = (struct Task *) arg;
#ifdef DEBUG
    printf(cBLUE"EVENT "cEND" OnEventDNSTask() id:%d %s %s\n", task->LObjId, getActionText(event), task->Record.HostName);
#endif
    if (event & EV_READ) {
        ares_process_fd(task->resolv->channel, fd, ARES_SOCKET_BAD);
    }
    ares_destroy(task->resolv->channel);
    event_del(&task->resolv->ev);
}

static void DNSTaskResolvCallback(void *arg, int status, int timeouts, unsigned char *abuf, int alen) {
    struct Task *task = (struct Task *) arg;
    int id, qr, opcode, aa, tc, rd, ra, rcode;
    u32 ip;
    unsigned int qdcount, ancount, nscount, arcount, i;
    const unsigned char *aptr;
#ifdef DEBUG
    printf(cBLUE"EVENT "cEND" DNSTaskCallBack() id:%d %s\n", task->LObjId, task->Record.HostName);
#endif
    if (status != ARES_SUCCESS) {
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
    for (i = 0; i < qdcount; i++) {
        aptr = skip_question(aptr, abuf, alen);
        if (aptr == NULL) return;
    }

    ip = ResolvAfterParseAnswer(aptr, abuf, alen);
    if (ip) task->Record.IP = ip;
}

void makeDNSTask(struct Task *task) {
    if (task->resolv->role == DNS_RESOLV) {
        ares_init(&task->resolv->channel);
        tv.tv_usec = 0;
        tv.tv_sec = config.timeout;
        ares_query(task->resolv->channel, task->resolv->task->Record.HostName, C_IN, T_A, DNSTaskResolvCallback, task);
        ares_getsock(task->resolv->channel, &task->resolv->fd, 1);
        event_set(&task->resolv->ev, task->resolv->fd, EV_READ | EV_TIMEOUT, OnEventDNSTask, task);
        event_add(&task->resolv->ev, &tv);
    } else {

    }
}

void timerDNSTask(int fd, short action, void *arg) {

    struct Task *task = (struct Task *) arg;
    if (!task->resolv) return;
    makeDNSTask(task);
#ifdef DEBUG
    printf(cBLUE"EVENT "cEND" timerDNSTask() id:%d %s %s\n", task->LObjId,
            getStatusText(task->code), task->Record.HostName);
#endif
    if (task->Record.ResolvePeriod) {
        tv.tv_sec = task->Record.ResolvePeriod;
        tv.tv_usec = 0;
        event_assign(&task->resolv->timer, base, -1, 0, timerDNSTask, task);
        evtimer_add(&task->resolv->timer, &tv);
    }

}

static void timerMain(int fd, short action, void *arg) {
    struct event *ev = (struct event *) arg;
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    evtimer_add(ev, &tv);

}

static void initDNSThread() {
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
    pthread_create(&threads, NULL, (void*) initDNSThread, NULL);
}


