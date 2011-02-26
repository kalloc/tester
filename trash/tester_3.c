#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <search.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <err.h>

#include <event2/event_struct.h>
#include <event2/event_compat.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>

#include <event2/event.h>
#include <time.h>

#include <ctype.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/queue.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>


struct Poll {
    struct event ev;
    struct bufferevent *bev;
    struct sockaddr_in client;
    unsigned status;
    char type;
    int fd;
};




#define u32 u_int32_t
#define FALSE 0
#define TRUE  1
struct timeval tv;
typedef struct st_server {
    struct Poll *poll;
    const char *host;
    int port;
    time_t timeout;

    struct bufferevent *bev;

    time_t periodRetrieve;
    time_t periodReport;

    struct event evConfig;
    struct event evReport;


} Server;




#pragma once

void OnBufferedError(struct bufferevent *bev, short what, void *arg) {
    Server *pServer = (Server *)arg;
    bufferevent_free(pServer->poll->bev);
    pServer->poll->status=0;
    gettimeofday(&tv, NULL);
    printf("OnBufferedError()  [time: %d]\n", tv.tv_sec);
}

void OnBufferedWrite(struct bufferevent *bev, void *arg) {
    gettimeofday(&tv, NULL);
    printf("OnBufferedWrite()  [time: %d]\n", tv.tv_sec);
}

void OnBufferedRead(struct bufferevent *bev, void *arg) {
    gettimeofday(&tv, NULL);
    printf("OnBufferedRead()  [time: %d]\n", tv.tv_sec);
}

void InitConnectTo(Server *pServer) {

    struct sockaddr_in sa;

    u32 ip;
    inet_aton(pServer->host, &ip);
    sa.sin_addr = *((struct in_addr *) & ip);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(pServer->port);
    bzero(&sa.sin_zero, 8);


    gettimeofday(&tv, NULL);
    printf("InitConnectTo()  [time: %d]\n", tv.tv_sec);

    pServer->poll->status = 1;
    timerclear(&tv);
     tv.tv_usec=pServer->timeout*1000*1000;

    pServer->poll->bev = bufferevent_socket_new(pServer->evConfig.ev_base, -1, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_socket_connect(pServer->poll->bev ,(struct sockaddr *)&sa, sizeof(sa));
    bufferevent_setcb(pServer->poll->bev, OnBufferedRead, OnBufferedWrite, OnBufferedError, pServer);
    bufferevent_enable(pServer->poll->bev, EV_READ|EV_WRITE |EV_TIMEOUT);
    bufferevent_set_timeouts(pServer->poll->bev, &tv, &tv);

}

void timerRetrieveTask(int fd, short action, void *arg) {
    Server *pServer = (Server *) arg;
    struct timeval tv;
    timerclear(&tv);
    tv.tv_sec = pServer->periodRetrieve;
    evtimer_add(&pServer->evConfig, &tv);

    gettimeofday(&tv, NULL);
    printf("timerRetrieveTask() -> [time %d]\n", (int) tv.tv_sec);

    if (pServer->poll->status == 0) {
        InitConnectTo(pServer);
    }

}


void timer(int fd, short action, void *arg) {
    Server *pServer = (Server *) arg;
    struct timeval tv;
    timerclear(&tv);
    tv.tv_sec = pServer->periodReport;
    evtimer_add(&pServer->evReport, &tv);
    gettimeofday(&tv, NULL);
    printf("timerSendReport() -> [time: %d]\n", tv.tv_sec);
}




int main(int argc, char **argv) {

    struct timeval tv;
    struct event_base *base;
    base=event_init();
    timerclear(&tv);

    Server *pServer;
    pServer=calloc(1,sizeof(*pServer));
    pServer->poll=calloc(1,sizeof(*pServer->poll));
    pServer->periodReport=1;
    pServer->host=strdup("1.1.1.1");
    pServer->port=80;
    pServer->periodRetrieve=10;
    pServer->periodReport=1;
    pServer->timeout=8;

    tv.tv_sec = pServer->periodReport;
    evtimer_set(&pServer->evReport, timer, pServer);
    evtimer_add(&pServer->evReport, &tv);

    tv.tv_sec = 0;
    evtimer_set(&pServer->evConfig, timerRetrieveTask, pServer);
    evtimer_add(&pServer->evConfig, &tv);


    event_dispatch();
    event_base_free(base);
    return 0;
}

