#include "include.h"
#include "yxml.h"
#include <zlib.h>
#include <ctype.h>
//#define STDOUT
struct timeval tv;
FILE * fp = 0;


//Authorization
int curve25519_donna(u8 *mypublic, const u8 *secret, const u8 *basepoint);

unsigned char * genSharedKey(Server *pServer, unsigned char * hispublic) {
    curve25519_donna(pServer->key.shared, pServer->key.secret, hispublic);
    return pServer->key.shared;
}

unsigned char * genPublicKey(Server *pServer) {
    static int count = 0;
    const unsigned char basepoint[32] = {9};
    if (!pServer->key.public[0] or count > 100) {
        FILE *fd;
        fd = fopen("/dev/urandom", "r");
        fread(pServer->key.secret, 32, 1, fd);
        fread(pServer->session.garbage, 16, 1, fd);
        fclose(fd);
        curve25519_donna(pServer->key.public, pServer->key.secret, basepoint);
        count = 0;
    } else {
        count += 1;
    }
    return pServer->key.public;
}


// BASE64
static const char base64digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#define BAD     -1
static const char base64val[] = {
    BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD,
    BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD,
    BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD, 62, BAD, BAD, BAD, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, BAD, BAD, BAD, BAD, BAD, BAD,
    BAD, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, BAD, BAD, BAD, BAD, BAD,
    BAD, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, BAD, BAD, BAD, BAD, BAD
};
#define DECODE64(c)  (isascii(c) ? base64val[c] : BAD)

void base64_encode(char *in, int inlen, char *out, int *outlen) {
    char *save_ptr;
    save_ptr = out;
    for (; inlen >= 3; inlen -= 3) {
        *out++ = base64digits[in[0] >> 2];
        *out++ = base64digits[((in[0] << 4) & 0x30) | (in[1] >> 4)];
        *out++ = base64digits[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
        *out++ = base64digits[in[2] & 0x3f];
        in += 3;
    }

    if (inlen > 0) {
        unsigned char fragment;

        *out++ = base64digits[in[0] >> 2];
        fragment = (in[0] << 4) & 0x30;

        if (inlen > 1)
            fragment |= in[1] >> 4;

        *out++ = base64digits[fragment];
        *out++ = (inlen < 2) ? '=' : base64digits[(in[1] << 2) & 0x3c];
        *out++ = '=';
    }

    *out = '\0';
    *outlen = ((int) out - (int) save_ptr);
}

int base64_decode(const unsigned char *in, unsigned char *out, int *outlen) {
    int len = 0;
    register unsigned char digit1, digit2, digit3, digit4;

    if (in[0] == '+' && in[1] == ' ')
        in += 2;
    if (*in == '\r') {
        outlen = 0;
        return (0);
    }
    u_char *ptr = out;
    do {
        digit1 = in[0];
        if (DECODE64(digit1) == BAD)
            return (-1);
        digit2 = in[1];
        if (DECODE64(digit2) == BAD)
            return (-1);
        digit3 = in[2];
        if (digit3 != '=' && DECODE64(digit3) == BAD)
            return (-1);
        digit4 = in[3];
        if (digit4 != '=' && DECODE64(digit4) == BAD)
            return (-1);
        in += 4;
        *out++ = (DECODE64(digit1) << 2) | (DECODE64(digit2) >> 4);
        ++len;
        if (digit3 != '=') {
            *out++ = ((DECODE64(digit2) << 4) & 0xf0) | (DECODE64(digit3) >> 2);
            ++len;
            if (digit4 != '=') {
                *out++ = ((DECODE64(digit3) << 6) & 0xc0) | DECODE64(digit4);
                ++len;
            }
        }
    } while (*in && *in != '\r' && digit4 != '=');
    *outlen = (int) out - (int) ptr;
    return 0;
}

// DNS
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

const char *type_name(int type) {
    int i;

    for (i = 0; i < ntypes; i++) {
        if (types[i].value == type)
            return types[i].name;
    }
    return "(unknown)";
}

const char *class_name(int dnsclass) {
    int i;

    for (i = 0; i < nclasses; i++) {
        if (classes[i].value == dnsclass)
            return classes[i].name;
    }
    return "(unknown)";
}

inline const unsigned char *skip_question(const unsigned char *aptr, const unsigned char *abuf, int alen) {
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

inline int getDNSTypeAfterParseAnswer(const unsigned char *aptr, const unsigned char *abuf, int alen) {
    int type, dnsclass, ttl, dlen, status;
    long len;

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
    return type;
}

inline const u32 getResolvedIPAfterParseAnswer(const unsigned char *aptr, const unsigned char *abuf, int alen) {
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
    hexPrint(abuf, alen);
    ares_free_string(name.as_char);

    if (type != T_A and type != T_NS) return 0;
    hexPrint(aptr, dlen);
    if (dlen != 4)
        return 0;
    return *(const u32 *) aptr;
    /*
        printf("\t%s\n", inet_ntop(AF_INET, aptr, addr, sizeof (addr)));
        hexPrint(addr, sizeof (addr));
        return addr;
     */
}



//LUA

void stackDump(lua_State *L, int line) {
    int i = lua_gettop(L);
    if (i == 0) return;
    printf("----------------  Stack Dump ---------%d-------\r\n", line);
    while (i) {
        int t = lua_type(L, i);
        switch (t) {
            case LUA_TSTRING:
                printf("string %d:`%s'\n", i, lua_tostring(L, i));
                break;
            case LUA_TBOOLEAN:
                printf("bool %d: %s\n", i, lua_toboolean(L, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:
                printf("num %d: %d\n", i, (int) lua_tonumber(L, i));
                break;
            default: printf("%d: %s %p\n", i, lua_typename(L, t), lua_topointer(L, t));
                break;
        }
        i--;
    }
    printf("--------------- Stack Dump Finished ---------------\r\n");
    fflush(stdout);
}


//Time

int timeDiffMS(struct timeval end, struct timeval start) {
    int time = (int) ((((double) end.tv_sec + (double) end.tv_usec / 1000000) - ((double) start.tv_sec + (double) start.tv_usec / 1000000))*1000);
    return time > 0x7ffe ? -1 : time;
}

int timeDiffUS(struct timeval end, struct timeval start) {
    return (int) ((((double) end.tv_sec + (double) end.tv_usec / 1000000) - ((double) start.tv_sec + (double) start.tv_usec / 1000000))*1000000);
}

double get_timeval_delta(struct timeval *after, struct timeval *before) {
    return ((double) after ->tv_usec - (double) before->tv_usec);
}


//Memory

inline void *getNulledMemory(int size) {
    /*
        void *ptr;
        ptr = malloc(size);
        bzero(ptr, size);
        return ptr;
     */
    return calloc(1, size);
}




//Config
FILE * fdConfig = NULL;
struct stat * statConfig = NULL;
char * bufConfig = 0;
yxml_t * xml = NULL;
yxml_t * tester_xml = NULL;
yxml_t * servers_xml = NULL;

unsigned int openConfiguration(char *filename) {


    int status = TRUE;
    const char * pstr;
    yxml_t * pxml = NULL;
    yxml_attr_t *xml_attr = NULL;
    int len = 0;
    bzero(&config, sizeof (config));


    statConfig = malloc(sizeof (* statConfig));
#ifdef DEBUG
    printf(cGREEN"Read Config File\n"cEND);
#endif
    if ((stat(filename, statConfig) < 0) or((int) (fdConfig = fopen(filename, "r")) < 0)) {
        printf(cRED"ERROR\n"cEND);
        printf(cGREEN"\tunable to load — "cEND""cBLUE"%s"cEND""cGREEN". System error — %s\n\n"cEND, filename, strerror(errno));
        status = FALSE;
    }

    if (status) {
        bufConfig = malloc(statConfig->st_size + 1);
        bzero(bufConfig, statConfig->st_size + 1);
        len = (int) fread((void *) bufConfig, statConfig->st_size, 1, fdConfig);
        xml = yxml_read(bufConfig, &pstr);

        if (xml == NULL) {
            printf(cRED"ERROR\n"cEND);
            printf(cGREEN"\tunable to parse xml - "cEND""cBLUE"%s"cEND"\n\n", filename);
            status = FALSE;
        }
        if (strcmp(xml->name, "config")) {
            status = FALSE;
        } else {
            xml = xml->details;
        }
    }

    if (status) {

        for (pxml = xml; pxml != 0; pxml = pxml->next) {
            if (!strcmp(pxml->name, "tester")) {
                tester_xml = pxml;
            } else if (!strcmp(pxml->name, "servers")) {
                servers_xml = pxml;
            }
        }
        if (!tester_xml and !servers_xml) return FALSE;

        for (xml_attr = tester_xml->attrs; xml_attr; xml_attr = xml_attr->next) {
            if (!strcmp(xml_attr->name, "id")) {
                config.testerid = atoi(xml_attr->value);
            } else if (!strcmp(xml_attr->name, "timeout")) {
                config.timeout = atoi(xml_attr->value);
            } else if (!strcmp(xml_attr->name, "maxread")) {
                config.maxInput = atoi(xml_attr->value);
            } else if (!strcmp(xml_attr->name, "path")) {
                config.lua.path = strdup((const char *) xml_attr->value);
            } else if (!strcmp(xml_attr->name, "log")) {
                config.log = strdup((const char *) xml_attr->value);
            } else if (!strcmp(xml_attr->name, "loglevel")) {
                config.loglevel = atoi(xml_attr->value);
            } else if (!strcmp(xml_attr->name, "minrecheck")) {
                config.minRecheckPeriod = atoi(xml_attr->value);
            } else if (!strcmp(xml_attr->name, "minperiod")) {
                config.minPeriod = atoi(xml_attr->value);
            }

        }
    }

    return status;
}

void closeConfiguration() {
    if (bufConfig) free(bufConfig);
    if (fdConfig) fclose(fdConfig);
    if (xml) yxml_free(xml);
    free(statConfig);
}

void loadServerFromConfiguration(Server *pServer, u32 skip) {

    yxml_t * pxml = NULL;
    yxml_attr_t *xml_attr = NULL;
    char key[50];
    u8 count = 0;

    for (pxml = servers_xml->details; pxml and count <= skip; pxml = pxml->next, count++) {
        if (!strcmp(pxml->name, "server")) {
            if (count < skip) continue;
            //извлекаем host и port
            for (xml_attr = pxml->attrs; xml_attr; xml_attr = xml_attr->next) {


                if (!strcmp(xml_attr->name, "host")) {
                    pServer->host = strdup((const char *) xml_attr->value);
                } else if (!strcmp(xml_attr->name, "port")) {
                    pServer->port = atoi(xml_attr->value);
                } else if (!strcmp(xml_attr->name, "key")) {
                    snprintf(pServer->session.password, 48, "%s", xml_attr->value);
                } else if (!strcmp(xml_attr->name, "keyRecv")) {
                    snprintf(pServer->passwordRecv, 48, "%s", xml_attr->value);
                } else if (!strcmp(xml_attr->name, "timeout")) {
                    pServer->timeout = atoi(xml_attr->value);
                } else if (!strcmp(xml_attr->name, "verifer")) {
                    config.pVeriferDC = pServer;
                } else if (!strcmp(xml_attr->name, "type")) {
                    if (!strcmp(xml_attr->value, "storage")) {
                        pServer->isSC = 1;
                        if (config.pServerSC) {

                            printf(cRED"ERROR\n"cEND);
                            printf(cGREEN"\t Only one StorageCenter Server in config.xml - "cEND"\n\n");
                            exit(0);

                        }
                        config.pServerSC = pServer;
                    } else if (!strcmp(xml_attr->value, "verifer")) {
                        pServer->isVerifer = 1;
                        if (config.pVerifer) {

                            printf(cRED"ERROR\n"cEND);
                            printf(cGREEN"\t Only one Verifer defined in config.xml - "cEND"\n\n");
                            exit(0);

                        }
                        config.pVerifer = pServer;
                    } else if (!strcmp(xml_attr->value, "data")) {
                        pServer->isDC = 1;
                    }
                } else if (!strcmp(xml_attr->name, "periodRetrieve")) {
                    pServer->periodRetrieve = atoi(xml_attr->value);
                } else if (!strcmp(xml_attr->name, "periodReport")) {
                    pServer->periodReport = atoi(xml_attr->value);
                } else if (!strcmp(xml_attr->name, "periodReportError")) {
                    pServer->periodReportError = atoi(xml_attr->value);
                }
            }
        }
    }
}


//Loger

void loger(char *codefile, char *codefunction, int level, const char *fmt, ...) {

    static pthread_mutex_t *mutex = NULL;
    if (config.loglevel & (level)) {
        if (!mutex) {
            mutex = calloc(1, sizeof (*mutex));
            pthread_mutex_init(mutex, NULL);
        }
        pthread_mutex_lock(mutex);

        int len, second, hour;
        char *buf, *file, *date;
        struct timeval tv;
        gettimeofday(&tv, NULL);
        buf = getNulledMemory(4096);
        if (fp == 0) {
            struct tm *tm;
            time_t result;
            result = time(NULL);
            file = getNulledMemory(4096);
            tm = localtime(&result);
            snprintf(file, 4096, config.log, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min);
            fp = fopen(file, "a");
            free(file);
        }
        if (!fp) return;
        second = tv.tv_sec - tv.tv_sec / 3600 * 3600;
        hour = tv.tv_sec / 3600 - (tv.tv_sec / 3600 / 24)*24;
        len = snprintf(buf, 4096, "[%02d:%02d:%02d %d] [%s%s%s%s] %s%s%s%s",
                hour, second / 60, second % 60, tv.tv_sec,
                level == LOG_DEBUG ? "DEBUG" : "",
                level == LOG_NOTICE ? "NOTICE" : "",
                level == LOG_INFO ? "INFO" : "",
                level == LOG_WARN ? "WARN" : "",
                codefile ? codefile : "",
                codefunction ? ":" : "",
                codefunction ? codefunction : "",
                codefunction ? "() " : ""
                );
        fwrite(buf, 1, len, fp);
#ifdef STDOUT
        printf(buf);
#endif
        va_list ap;
        va_start(ap, fmt);
        len = vsnprintf(buf, 4094, fmt, ap);
        buf[len] = '\n';
        buf[len + 1] = 0;
        fwrite(buf, 1, len + 1, fp);
        fflush(fp);
        va_end(ap);
#ifdef STDOUT
        printf(buf);
#endif
        free(buf);
        pthread_mutex_unlock(mutex);
    }

}


//Reload

void restart_handler(int signum) {
    if (fp != 0) fclose(fp);
    fp = 0;
}


//Network

void closeConnection(Server *pServer, short needDelete) {
    struct Poll * poll = pServer->poll;

    if (poll->bev) {
        bufferevent_free(poll->bev);
    }
#ifdef DEBUG
    if (poll->type == MODE_SERVER) {
        debug("MODE_SERVER %s:%d", pServer->host, pServer->port);
    } else {
        debug("%s/%s", getStatusText(poll->status), getActionText(poll->type));
    }
#endif
    poll->status = STATE_DISCONNECTED;
    if (poll->fd) {
        shutdown(poll->fd, 1);
        close(poll->fd);
        poll->fd = 0;
    }

    if (needDelete) {
        if ((char *) & poll->ev != 0) {
            event_del(&poll->ev);
        }
        free(poll);
    }
}

void inline setNextTimer(struct Task *task) {
    timerclear(&tv);
    if (task->Record.CheckPeriod and task->pServer->timeOfLastUpdate == task->timeOfLastUpdate) {
        if (task->newTimer > 0) {
            debug("Remainder before activate id:%d %d", task->LObjId, task->Record.NextCheckDt);
            tv.tv_sec = task->newTimer;
            task->newTimer = 0;
            task->isShiftActive = 1;
            debug("Remainder activate id:%d %d -> %d", task->LObjId, tv.tv_sec, task->Record.NextCheckDt);
        } else {
            task->isShiftActive = 0;
            tv.tv_sec = task->Record.CheckPeriod;
        }
    } else {
        task->isShiftActive = 0;
        tv.tv_sec = 60;
        task->isEnd = TRUE;
    }
    evtimer_add(&task->time_ev, &tv);
}

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

//String

void strtolower(char *str) {
    for (; *str; str++) {
        if (*str >= 'A' && *str <= 'Z') {
            *str += 32;
        } else if (*str >= 192 && *str <= 223) {
            *str += 32;
        }
    }
}

char * ConnectedIpString(Server *pServer) {
    static char IP[23];
    char *ptr;
    if (!(ptr = inet_ntoa(pServer->poll->client.sin_addr))) {
        return "UNKNOW";
    }
    snprintf(IP, 23, "%s:%d", ptr, pServer->poll->client.sin_port);
    return IP;


}

char * ipString(u32 net) {
    char *ptr;
    struct in_addr in;
    in.s_addr = (in_addr_t) net;
    ptr = inet_ntoa(in);
    return ptr ? ptr : "UNKNOW";
}

char * getActionText(int code) {
    static char buf[100];
    snprintf(buf, 100, "%s%s%s [%x]",
            code & EV_READ ? "EV_READ " : "",
            code & EV_WRITE ? "EV_WRITE " : "",
            code & EV_TIMEOUT ? "EV_TIMEOUT " : "", code
            );
    return buf;
}

char * getErrorText(int code) {
    switch (code) {
        case SOCK_RC_TIMEOUT:
            return "SOCK_RC_TIMEOUT";
        case SOCK_RC_AUTH_FAILURE:
            return "SOCK_RC_AUTH_FAILURE";
        case SOCK_RC_WRONG_SIZES:
            return "SOCK_RC_WRONG_SIZES";
        case SOCK_RC_NOT_ENOUGH_MEMORY:
            return "SOCK_RC_NOT_ENOUGH_MEMORY";
        case SOCK_RC_CRC_ERROR:
            return "SOCK_RC_CRC_ERROR";
        case SOCK_RC_DECOMPRESS_FAILED:
            return "SOCK_RC_DECOMPRESS_FAILED";
        case SOCK_RC_INTEGRITY_FAILED:
            return "SOCK_RC_INTEGRITY_FAILED";
        case SOCK_RC_SQL_ERROR:
            return "SOCK_RC_SQL_ERROR";
        case SOCK_RC_UNKNOWN_REQUEST:
            return "SOCK_RC_UNKNOWN_REQUEST";
        case SOCK_RC_TOO_MUCH_CONNECTIONS:
            return "SOCK_RC_TOO_MUCH_CONNECTIONS";
        case SOCK_RC_SERVER_SHUTTING_DOWN:
            return "SOCK_RC_SERVER_SHUTTING_DOWN";
        case TESTER_SQL_HOST_NAME_LEN:
            return "TESTER_SQL_HOST_NAME_LEN";
        default:
            return "UNKNOW";
    }
}

char * getModuleText(int code) {
    switch (code) {
        case MODULE_PING:
            return "MODULE_PING";
        case MODULE_TCP_PORT:
            return "MODULE_TCP_PORT";
        case MODULE_HTTP:
            return "MODULE_HTTP";
        case MODULE_SMTP:
            return "MODULE_SMTP";
        case MODULE_FTP:
            return "MODULE_FTP";
        case MODULE_DNS:
            return "MODULE_DNS";
        case MODULE_POP:
            return "MODULE_POP";
        case MODULE_TELNET:
            return "MODULE_TELNET";
        case MODULE_READ_OBJECTS_CFG:
            return "MODULE_READ_OBJECTS_CFG ";
        default:
            return "UNKNOW";
    }
}

char * getRoleText(int code) {
    switch (code) {
        case DNS_SUBTASK:
            return "DNS_SUBTASK";
        case DNS_RESOLV:
            return "DNS_RESOLV";
        case DNS_TASK:
            return "DNS_TASK";
        case DNS_GETNS:
            return "DNS_GETNS";
        default:
            return "UNKNOW";
    }
}

char * getStatusText(int code) {
    static char buf[1024];
    snprintf(buf, 1024, "%s%s%s%s%s%s%s%s%s%s [%d]",
            code & STATE_CONNECTED ? "STATE_CONNECTED " : "",
            code & STATE_CONNECTING ? "STATE_CONNECTING " : "",
            code == STATE_DISCONNECTED ? "STATE_DISCONNECTED " : "",
            code & STATE_DONE ? "STATE_DONE " : "",
            code & STATE_ERROR ? "STATE_ERROR " : "",
            code & STATE_READ ? "STATE_READ " : "",
            code & STATE_WRITE ? "STATE_WRITE " : "",
            code & STATE_QUIT ? "STATE_QUIT " : "",
            code & STATE_TIMEOUT ? "STATE_TIMEOUT " : "",
            code & STATE_SESSION ? "STATE_SESSION " : "",
            code
            );
    return buf;
}

int hex2bin(char *hex, char *bin) {

    return 0;
}

unsigned char * bin2hex(unsigned char *bin, int len) {
    static u_char hex[BUFLEN];
    bzero(&hex, BUFLEN);
    len -= 1;
    u_char left = 0, right = 0;
    hex[len * 2 + 2] = 0;
    for (; len >= 0; len--) {
        left = bin[len] >> 4;
        right = bin[len]-(bin[len] >> 4 << 4);

        if (right >= 0 and right <= 9) {
            hex[len * 2 + 1] = right + 48;
        } else {
            hex[len * 2 + 1] = right + 87;
        }
        if (left >= 0 and left <= 9) {
            hex[len * 2] = left + 48;
        } else {
            hex[len * 2] = left + 87;
        }
    }
    return hex;

}

void hexPrint(char *inPtr, int inLen) {
#define LINE_LEN 16
    int sOffset = 0;
    u16 Index = 0;
    for (Index = 0; inLen > 0; inLen -= LINE_LEN, sOffset += LINE_LEN) {

        printf("  %05d: ", sOffset);

        for (Index = 0; Index < LINE_LEN; Index++) {
            if (Index < inLen) {
                printf("%02x ", (unsigned char) inPtr[Index + sOffset]);
            } else {
                printf("   ");
            }
        }

        printf(" : ");

        for (Index = 0; Index < LINE_LEN; Index++) {
            char byte = ' ';

            if (Index < inLen) {
                byte = inPtr[Index + sOffset];
            }
            printf("%c", (((byte & 0x80) == 0) && isprint(byte)) ? byte : '.');
        }

        printf("\n");

    }
}

#ifdef TEST

// строка для теста

int main(int argc, char **argv) {
    char ptask[40000], ptr2[40000];
    char Plain[] = "Some Some Chars";
    int len, fd;
    char public1[32], public2[32], private1[32], private2[32], shared1[32], shared2[32];
    unsigned char basepoint1[32] = {9}, basepoint2[32] = {9};
    printf("Test Curve\n");

    fd = fopen("/dev/urandom", "r");
    fread(private1, 32, 1, fd);
    fread(private2, 32, 1, fd);
    fclose(fd);
    bzero(&basepoint1, 32);
    basepoint1[0] = 9;
    bzero(&basepoint2, 32);
    basepoint2[0] = 9;
    curve25519_donna(public1, private1, basepoint1);
    curve25519_donna(public2, private2, basepoint2);

    curve25519_donna(shared1, private1, public2);
    curve25519_donna(shared2, private2, public1);

    printf("Shared1: %s\n", bin2hex(shared1, 32));
    printf("Public1: %s\n", bin2hex(public1, 32));
    printf("Private1: %s\n", bin2hex(private1, 32));

    printf("Shared2: %s\n", bin2hex(shared2, 32));
    printf("Public2: %s\n", bin2hex(public2, 32));
    printf("Private2: %s\n", bin2hex(private2, 32));

    return 0;


}


#endif