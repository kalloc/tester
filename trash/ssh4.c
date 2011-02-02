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

struct timeval tv;
unsigned counter = 0;
unsigned start=0;
unsigned end=0;
unsigned maxfd=0;


#define BUFLEN 4096
#define u16 u_int16_t

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
    bufferevent_free(bev);
}

void OnBufferedWrite(struct bufferevent *bev, void *arg) {
    struct Poll *poll=(struct Poll *)arg;
//    bufferevent_write(bev,Request,sizeof(Request));
    bufferevent_enable(bev,EV_READ);
}

void OnBufferedRead(struct bufferevent *bev, void *arg) {
    struct Poll *poll=(struct Poll *)arg;
    struct evbuffer *buffer = EVBUFFER_INPUT(bev);
    u_char *data = EVBUFFER_DATA(buffer);
    u_int len = EVBUFFER_LENGTH(buffer);
    hexPrint(data,len);
}






int main(int argc, char **argv) {
    event_init();
    struct sockaddr_in sa;
    struct bufferevent *bev;
    int fd;
    unsigned ip;
    inet_aton("213.248.62.7",&ip);
    sa.sin_addr = *((struct in_addr *) &ip);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(22);
    bzero(&sa.sin_zero, 8);
    fd=socket(AF_INET, SOCK_STREAM, 0);
    setnb(fd);
    connect(fd, (struct sockaddr *) & sa, sizeof (struct sockaddr));
    bev = bufferevent_new(fd, OnBufferedRead, OnBufferedWrite, OnBufferedError, NULL);
    bufferevent_enable(bev, EV_READ);
    bufferevent_settimeout(bev,10,10);
    event_dispatch();
    return 0;
}


