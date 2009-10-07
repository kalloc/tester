#include "include.h"
#include "yxml.h"
#include <zlib.h>
#include <ctype.h>

int curve25519_donna(u8 *mypublic, const u8 *secret, const u8 *basepoint);

FILE * fp = 0;



static const char base64digits[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

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

int timeDiffMS(struct timeval end, struct timeval start) {
    int time = (int) ((((double) end.tv_sec + (double) end.tv_usec / 1000000) - ((double) start.tv_sec + (double) start.tv_usec / 1000000))*1000);
    return time > 0x7ffe ? -1 : time;
}

int timeDiffUS(struct timeval end, struct timeval start) {
    return (int) ((((double) end.tv_sec + (double) end.tv_usec / 1000000) - ((double) start.tv_sec + (double) start.tv_usec / 1000000))*1000000);
}

inline void *getNulledMemory(int size) {
    /*
        void *ptr;
        ptr = malloc(size);
        bzero(ptr, size);
        return ptr;
     */
    return calloc(1, size);
}
FILE * fdConfig = NULL;
struct stat * statConfig = NULL;
char * bufConfig = 0;
yxml_t * xml = NULL;

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
    }

    if (status) {
        for (pxml = xml; pxml != 0; pxml = pxml->next) {
            if (!strcmp(pxml->name, "tester")) {
                //извлекаем key, period и timeout
                for (xml_attr = pxml->attrs; xml_attr; xml_attr = xml_attr->next) {
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
                    } else if (!strcmp(xml_attr->name, "minrecheck")) {
                        config.minRecheckPeriod = atoi(xml_attr->value);
                    } else if (!strcmp(xml_attr->name, "minperiod")) {
                        config.minPeriod = atoi(xml_attr->value);
                    }

                }
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

void loadServerFromConfiguration(Server *pServer, u32 ServerId) {

    yxml_t * pxml = NULL;
    yxml_attr_t *xml_attr = NULL;
    char key[50];
    if (ServerId == 0) {
        snprintf((char *) & key, 50, "server");
    } else {
        snprintf((char *) & key, 50, "server%d", ServerId);
    }
    for (pxml = xml; pxml != 0; pxml = pxml->next) {
        //ключ server
        if (!strcmp(pxml->name, key)) {
            //извлекаем host и port
            for (xml_attr = pxml->attrs; xml_attr; xml_attr = xml_attr->next) {
                if (!strcmp(xml_attr->name, "host")) {
                    pServer->host = strdup((const char *) xml_attr->value);
                } else if (!strcmp(xml_attr->name, "port")) {
                    pServer->port = atoi(xml_attr->value);
                } else if (!strcmp(xml_attr->name, "key")) {
                    snprintf(pServer->session.password, 42, "%s", xml_attr->value);
                } else if (!strcmp(xml_attr->name, "timeout")) {
                    pServer->timeout = atoi(xml_attr->value);
                } else if (!strcmp(xml_attr->name, "type")) {
                    if (!strcmp(xml_attr->value, "storage")) {
                        pServer->isSC = 1;
                        if (config.pServerSC) {

                            printf(cRED"ERROR\n"cEND);
                            printf(cGREEN"\t Only one StorageCenter Server in config.xml - "cEND"\n\n");
                            exit(0);

                        }
                        config.pServerSC = pServer;
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


/*
Data makeData(char *ptr, u_int len) {
    static Data data;
    data.ptr = ptr;
    data.len = len;
    return data;
}
 */

/*
static long long rdtsc(void) {
    long long l;
    asm volatile("rdtsc\r\n" : "=A" (l));
    return l;
}
 */

double get_timeval_delta(struct timeval *after, struct timeval *before) {
    return ((double) after ->tv_usec - (double) before->tv_usec);
}

int setb(int fd) {
    int flags;
    flags = fcntl(fd, F_GETFL);
    if (flags < 0) return flags;

    flags ^= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) return -1;

    return 0;
}

void loger(char *codefile, char *codefunction, int level, const char *fmt, ...) {

    /*
        static pthread_mutex_t *mutex = NULL;
        if (!mutex) {
            mutex = calloc(1, sizeof (*mutex));
            pthread_mutex_init(mutex, NULL);
        }
      pthread_mutex_lock(mutex);
     */

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
        snprintf(file, 4096, config.log, 1900 + tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min);
        fp = fopen(file, "a");
        free(file);
    }
    if(!fp) return;
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
    va_list ap;
    va_start(ap, fmt);
    len = vsnprintf(buf, 4094, fmt, ap);
    buf[len] = '\n';
    buf[len + 1] = 0;
    fwrite(buf, 1, len + 1, fp);
    fflush(fp);
    va_end(ap);
    free(buf);
    //    pthread_mutex_unlock(mutex);
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

void restart_handler(int signum) {
    if (fp != 0) fclose(fp);
    fp = 0;
}

void strtolower(char *str) {
    for (; *str; str++) {
        if (*str >= 'A' && *str <= 'Z') {
            *str += 32;
        } else if (*str >= 192 && *str <= 223) {
            *str += 32;
        }
    }
}

char * getActionText(int code) {
    static char buf[100];
    snprintf(buf, 100, "%s%s%s [%d]",
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

void
closeConnection(Server *pServer, short needDelete) {
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

/* in_cksum from ping.c --
 *      Checksum routine for Internet Protocol family headers (C Version)
 *
 * Copyright (c) 1989, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Muuss.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
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

#ifdef TEST

// строка для теста

int main(int argc, char **argv) {
    char ptr[40000], ptr2[40000];
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