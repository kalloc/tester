#include <stdio.h>
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
#include <libssh2.h>
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
char username[]="root";
char password[]="gbpltw666cbcntvt";

int main() {
    unsigned char *ptr,*hex,*banner;
    const char *fingerprint;
    char *userauthlist;
    unsigned char readbuf[BUFLEN];
    int fd=0,ip,state,len,auth,i,auth_pw;
    struct sockaddr_in sa;

    inet_aton("213.248.62.7", (struct in_addr *) & ip);
    sa.sin_addr = *((struct in_addr *) & ip);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(22);
    bzero(&sa.sin_zero, 8);

    fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    fcntl(fd, F_SETFL, 0);
    connect(fd, (struct sockaddr *) & sa, sizeof (struct sockaddr));
    LIBSSH2_SESSION *ssh = libssh2_session_init();
    LIBSSH2_CHANNEL *channel;
//    libssh2_session_set_blocking(ssh,0);
    libssh2_session_startup(ssh, fd);
    libssh2_trace(ssh, ~0 );
    fingerprint = libssh2_hostkey_hash(ssh, LIBSSH2_HOSTKEY_HASH_MD5);
    printf("Fingerprint: ");
    for(i = 0; i < 16; i++) {
	printf("%02X ", (unsigned char)fingerprint[i]);
    }
    printf("\n");

    /* check what authentication methods are available */
    userauthlist = libssh2_userauth_list(ssh, username, strlen(username));
    printf("Authentication methods: %s\n", userauthlist);
    if (strstr(userauthlist, "password") != NULL) {
	auth_pw |= 1;
    }
    if (strstr(userauthlist, "keyboard-interactive") != NULL) {
	auth_pw |= 2;
    }
    if (strstr(userauthlist, "publickey") != NULL) {
	auth_pw |= 4;
    }
    if (libssh2_userauth_password(ssh, username, password)) {
        printf("\tAuthentication by password failed!\n");
    } else {
        printf("\tAuthentication by password succeeded.\n");
    }
    channel = libssh2_channel_open_session(ssh);
    libssh2_channel_exec(channel, "/bin/ls -al /");
    len=libssh2_channel_read(channel, readbuf, sizeof(readbuf));
    write(0,readbuf,len);
    libssh2_session_disconnect(ssh,"Normal Shutdown, Thank you for playing");
    libssh2_session_free(ssh);

//    ssh_userauth_password(ssh,"root","gbpltw666cbcntvt");
}

