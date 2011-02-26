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
#include <event.h>
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


#define cBLUE		"\e[1;34m"
#define cYELLOW		"\e[1;33m"
#define cGREEN		"\e[1;32m"
#define cRED		"\e[1;31m"
#define cEND		"\e[0m"

FILE *fp;

struct Poll {
    unsigned int IP;
    struct bufferevent *bev;
    int fd;
    unsigned int port;
};

struct timeval tv;
unsigned counter = 0;
unsigned start=0;
unsigned end=0;
char Request[]="GET http://ya.ru HTTP/1.1\r\nHost:ya.ru\r\nConnection:close\r\n\r\n";
char *Pattern;
int PatternLen;
unsigned maxfd=0;

void addTask();



char * ipString(unsigned  net) {
    char *ptr;
    struct in_addr in;
    in.s_addr = (in_addr_t) net;
    ptr = inet_ntoa(in);
    return ptr ? ptr : "UNKNOW";
}
                    
int setnb(int fd) {
    int flags;
    flags = fcntl(fd, F_GETFL);
    if (flags < 0) return flags;
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) return -1;
    return 0;

}



void OnBufferedError(struct bufferevent *bev, short what, void *arg) {
    struct Poll *poll=(struct Poll *)arg;
    counter--;
    bufferevent_free(poll->bev);
    shutdown(poll->fd,1);
    close(poll->fd);
    free(poll);
    addTask();
}

void OnBufferedWrite(struct bufferevent *bev, void *arg) {
    struct Poll *poll=(struct Poll *)arg;
    write(poll->fd,Request,sizeof(Request));
//    fprintf(stderr,cGREEN" WRITE TO %s:%d\n"cEND,ipString(poll->IP),poll->port);
    bufferevent_disable(bev,EV_WRITE);
    bufferevent_enable(bev,EV_READ);
    addTask();
}

void OnBufferedRead(struct bufferevent *bev, void *arg) {
    struct Poll *poll=(struct Poll *)arg;
    struct evbuffer *buffer = EVBUFFER_INPUT(bev);
    u_char *data = EVBUFFER_DATA(buffer);
    u_int len = EVBUFFER_LENGTH(buffer);
        fprintf(stderr,cBLUE"%s:%d - "cEND,ipString(poll->IP),poll->port);
    if(evbuffer_find(buffer,Pattern,PatternLen)) {
	fprintf(fp,"%s:%d\n",ipString(poll->IP),poll->port);
	fflush(fp);
	fprintf(stderr,cGREEN" PROXY\n"cEND);
    } else {
        fprintf(stderr,cRED"  NO"cEND);
    }
    
    OnBufferedError(bev, EV_TIMEOUT,arg);
}



void ConnectTo(unsigned ip,unsigned port) {

    struct Poll *poll=malloc(sizeof(*poll));
    struct sockaddr_in sa;
    poll->IP=ip;
    poll->port=port;
    sa.sin_addr = *((struct in_addr *) &ip);
//    fprintf(stderr,cGREEN" CONNECT TO %s:%d\n"cEND,ipString(ip),poll->port);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    bzero(&sa.sin_zero, 8);
    poll->fd=socket(AF_INET, SOCK_STREAM, 0);
    setnb(poll->fd);
    connect(poll->fd, (struct sockaddr *) & sa, sizeof (struct sockaddr));
    poll->bev = bufferevent_new(poll->fd, OnBufferedRead, OnBufferedWrite, OnBufferedError, poll);
    bufferevent_enable(poll->bev, EV_WRITE);
    bufferevent_settimeout(poll->bev,10,10);
    counter++;

}

void addTask() {
    int step=0;
    while(start <= end && counter < maxfd && step < 500) {
	ConnectTo(htonl(start),8080);
	ConnectTo(htonl(start),3128);
	ConnectTo(htonl(start),1080);
	ConnectTo(htonl(start),80);
	start++;
	step++;
    }
}


int main(int argc, char **argv) {
    struct rlimit rlim;
    getrlimit(RLIMIT_NOFILE,&rlim);
    // возможно лучше проверять не сдох ли fd
    struct sigaction IgnoreYourSignal;
    sigemptyset(&IgnoreYourSignal.sa_mask);
    IgnoreYourSignal.sa_handler = SIG_IGN;
    IgnoreYourSignal.sa_flags = 0;
    sigaction(SIGPIPE, &IgnoreYourSignal, NULL);
                        
    maxfd=rlim.rlim_cur/4;
    fp=fopen("log.log","a");
    if(argc < 4) {
        fprintf(stderr,cRED"\n\tUsage %s 192.168.0.0 192.168.255.255 keyword\n\n"cEND,argv[0]);
	exit(0);
    }

    event_init();
    fprintf(stderr,cGREEN"Start from %s to %s\n"cEND,argv[1],argv[2]);
    inet_aton(argv[1],(struct in_addr *)&start);
    start=ntohl(start);
    inet_aton(argv[2],(struct in_addr *)&end);
    end=ntohl(end);
    Pattern=strdup(argv[3]);
    PatternLen=strlen(Pattern);
    addTask();
    event_dispatch();
    fprintf(stderr,cGREEN"END\n"cEND);
    fclose(fp);
    return 0;
}



// gcc proxy_scan.c -levent -o ./proxy_scan &&   time ./proxy_scan  192.168.1.0 192.168.1.230 HTTP

