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
#include <libssh/libssh.h>
#define BUFLEN 4096

unsigned char * bin2hex(unsigned char *bin, int len) {
    static u_char hex[BUFLEN];
    bzero(&hex, BUFLEN);
    len -= 1;
    u_char left = 0, right = 0;
    hex[len * 2 + 2] = 0;
    for (; len >= 0; len--) {
        left = bin[len] >> 4;
        right = bin[len]-(bin[len] >> 4 << 4);

        if (right >= 0 && right <= 9) {
            hex[len * 2 + 1] = right + 48;
        } else {
            hex[len * 2 + 1] = right + 87;
        }
        if (left >= 0 && left <= 9) {
            hex[len * 2] = left + 48;
        } else {
            hex[len * 2] = left + 87;
        }
    }
    return hex;

}



int main() {
    unsigned char *ptr,*hex,*banner;
    unsigned char readbuf[BUFLEN];
    int fd=0,ip,state,len,auth;
    struct sockaddr_in sa;

    inet_aton("213.248.62.7", (struct in_addr *) & ip);
    sa.sin_addr = *((struct in_addr *) & ip);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(22);
    bzero(&sa.sin_zero, 8);

    fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    fcntl(fd, F_SETFL, 0);
    connect(fd, (struct sockaddr *) & sa, sizeof (struct sockaddr));
    SSH_SESSION *ssh = ssh_new();
    SSH_OPTIONS *ssh_options = ssh_options_new();
    ssh_options_set_fd(ssh_options,fd);
    ssh_set_options(ssh,ssh_options);
    ssh_connect(ssh);
    len=ssh_get_pubkey_hash(ssh,&ptr);
    printf("hash is %s\n",bin2hex(&ptr,len));
    ssh_userauth_password(ssh,"root","gbpltw666cbcntvt");
    CHANNEL *channel = channel_new(ssh);
    channel_open_session(channel);
    channel_request_exec(channel,"/bin/ls");
    channel_read(channel,(void *)readbuf,40,0);
    printf("%s\n",readbuf);
    close(fd);
}


