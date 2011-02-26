/*
 *  Copyright (c) 2006 Omachonu Ogali <oogali@idlepattern.com>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libssh2.h>
#include <getopt.h>

void usage(void)
{
	fprintf(stderr, "cmdpw [-lp] hostname password command_to_run\n");
	fprintf(stderr, "-l\tUsername to login with\n");
	fprintf(stderr, "-p\tPort to connect to\n");
	exit(0);
}

int main(int argc, char **argv)
{
	int s;
	int o;
	int port;
	int len;
	int elen;
	int readlen;
	char *ip = NULL;
	char *auth = NULL;
	char *errmsg;
	char *host = NULL;
	char *user = NULL;
	char *ptr = NULL;
	char hostkey[16];
	char cmdbuf[16384];
	LIBSSH2_SESSION *ssh;
	LIBSSH2_CHANNEL *sshc;
	struct hostent *h = NULL;
	struct servent *se = NULL;
	struct sockaddr_in sin;
	static struct option longopts[] = {
		{ "login",	required_argument,	NULL,	'l' },
		{ "port",	required_argument,	NULL,	'p' }
	};

	port = 0;
	user = NULL;
	host = NULL;

	while ((o = getopt_long(argc, argv, "l:p:", longopts, NULL)) != -1) {
		switch(o) {
			case 'l':
				len = strlen(optarg);
				if ((user = malloc(len)) == NULL) {
					perror("malloc");
					return -1;
				}

				strncpy(user, optarg, len);
				break;
			case 'p':
				port = strtoul(optarg, (char **)NULL, 10);
				if (port == 0 || port > 65535) {
					fprintf(stderr, "Invalid port specified\n");
					return -1;
				}
				break;
			default:
				usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 3) {
		usage();
	}

	if (user == NULL) {
		ptr = strstr(argv[0], "@");
		if (ptr == NULL) {
			user = malloc(strlen(getlogin()));
			if (user == NULL) {
				perror("malloc");
				return -1;
			}

			strncpy(user, getlogin(), strlen(getlogin()));
			host = argv[0];
		} else {
			len = (strlen(argv[0]) - strlen(ptr)) + 1;
			if ((user = malloc(len)) == NULL) {
				perror("malloc");
				return -1;
			}

			strncpy(user, argv[0], len);
			user[len - 1] = '\0';

			host = ptr + 1;
		}
	} else {
		host = argv[0];
	}

	if (port == 0) {
		ptr = strstr(host, ":");
		if (ptr == NULL) {
			if ((se = getservbyname("ssh", "tcp")) == NULL) {
				perror("getservbyname");
				return -1;
			}

			port = ntohs(se->s_port);
		} else {
			port = strtoul(ptr + 1, (char **)NULL, 10);
			if (port == 0 || port > 65535) {
				fprintf(stderr, "Invalid port specified");
				return -1;
			}

			host[strlen(host) - strlen(ptr)] = '\0';
		}
	}

	if (host == NULL) {
		host = argv[0];
	}

	h = gethostbyname(host);
	if (h == NULL) {
		herror("gethostbyname");
		return -1;
	}

	bzero(&sin, sizeof(struct sockaddr));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	memcpy(&sin.sin_addr, h->h_addr, sizeof(struct in_addr));
	ip = inet_ntoa(sin.sin_addr);

	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket");
		return -1;
	}

	fprintf(stderr, "Connecting to %s [%s], port %d as user %s\n\n", host, ip, port, user);
	if (connect(s, (struct sockaddr *)&sin, sizeof(struct sockaddr)) < 0) {
		perror("connect");
		return -1;
	}

	if ((ssh = libssh2_session_init()) == NULL) {
		fprintf(stderr, "Could not initialize ssh session\n");
		return -1;
	}

	if (libssh2_session_startup(ssh, s) < 0) {
		fprintf(stderr, "Could not associate socket with session\n");
		return -1;
	}

	memcpy(hostkey, libssh2_hostkey_hash(ssh, LIBSSH2_HOSTKEY_HASH_MD5), sizeof(hostkey));
	if (hostkey == NULL) {
		fprintf(stderr, "Could not get hostkey\n");
		return -1;
	}

	fprintf(stderr, "K:%s,E:%s,M:%s,C:%s\n", \
		libssh2_session_methods(ssh, LIBSSH2_METHOD_KEX),
		libssh2_session_methods(ssh, LIBSSH2_METHOD_CRYPT_CS),
		libssh2_session_methods(ssh, LIBSSH2_METHOD_MAC_CS),
		libssh2_session_methods(ssh, LIBSSH2_METHOD_COMP_CS));

	fprintf(stderr, "HK:%s/%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", \
		libssh2_session_methods(ssh, LIBSSH2_METHOD_HOSTKEY), \
		hostkey[0] & 0xff, hostkey[1] & 0xff, \
		hostkey[2] & 0xff, hostkey[3] & 0xff, \
		hostkey[4] & 0xff, hostkey[5] & 0xff, \
		hostkey[6] & 0xff, hostkey[7] & 0xff, \
		hostkey[8] & 0xff, hostkey[9] & 0xff, \
		hostkey[10] & 0xff, hostkey[11] & 0xff, \
		hostkey[12] & 0xff, hostkey[13] & 0xff, \
		hostkey[14] & 0xff, hostkey[15] & 0xff);

	auth = libssh2_userauth_list(ssh, user, strlen(user));
	if (auth == NULL) {
		if (libssh2_userauth_authenticated(ssh) == 0) {
			fprintf(stderr, "Could not get list of authentication methods\n");
			return -1;
		} else {
			fprintf(stderr, "SERVER ACCEPTS NULL AUTHENTICATION\n");
		}
	} else {
		fprintf(stderr, "A:%s\n", auth);
	}

	if (libssh2_userauth_password(ssh, user, argv[1]) < 0) {
		fprintf(stderr, "Login unsuccessful\n");
		libssh2_session_free(ssh);
		free(user);
		close(s);

		return -1;
	} else {
		fprintf(stderr, "Successfully logged in\n");
	}

	sshc = libssh2_channel_open_session(ssh);
	if (sshc == NULL) {
		fprintf(stderr, "Failed at requesting a channel\n");
		return -1;
	}

	libssh2_channel_set_blocking(sshc, 1);
	if (libssh2_channel_exec(sshc, argv[2]) < 0) {
		fprintf(stderr, "Failed at requesting command\n");
		return -1;
	}

	while ((readlen = libssh2_channel_read(sshc, cmdbuf, sizeof(cmdbuf))) > 0) {
		printf("%s\n", cmdbuf);
	}

	if (libssh2_channel_close(sshc) < 0) {
		fprintf(stderr, "Could not close channel\n");
		return -1;
	}

	if (libssh2_session_disconnect(ssh, "Why not?") < 0) {
		fprintf(stderr, "Could not successfully disconnect session\n");
		return -1;
	}

	libssh2_session_free(ssh);
	free(user);

	close(s);
	return 0;
}
