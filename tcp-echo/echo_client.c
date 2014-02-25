
/*
 * echo_client.c
 *
 * Stream (TCP) echo client for use with an echo server.
 *
 *   (terminal 1)
 *   ./echo_server
 *   Port: 16245
 *
 *   (terminal 2)
 *   ./echo_client localhost 16245
 *   (type in characters and the result will be echoed back)
 *
 * Author:
 *   Jeremiah Mahler <jmmahler@gmail.com>
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXLINE 1500

int sockfd = 0;
struct addrinfo *res = NULL;

void cleanup(void) {
	if (sockfd > 0)
		close(sockfd);

	if (res)
		freeaddrinfo(res);
}

int main(int argc, char* argv[]) {
	struct addrinfo hints, *p;
	int n;
	char *host;
	char *port;
	char buf[MAXLINE];

	/* configure at exit cleanup function */
	sockfd = 0;
	res = NULL;
	if (atexit(cleanup) != 0) {
		fprintf(stderr, "unable to set exit function\n");
		exit(EXIT_FAILURE);
	}
	signal(SIGINT, exit);  /* catch Ctrl-C/Ctrl-D and exit */

	if (argc != 3) {
		fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	host = argv[1];
	port = argv[2];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_INET;
	hints.ai_socktype 	= SOCK_STREAM;

	if ( (n = getaddrinfo(host, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
		exit(EXIT_FAILURE);
	}

	/* loop through addresses, use first one that works */
	for (p = res; p != NULL; p = p->ai_next) {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (-1 == sockfd) {
			perror("socket");
			continue;
		}

		break;
	}
	if (NULL == p) {
		fprintf(stderr, "No address found.\n");
		exit(EXIT_FAILURE);
	}

	if (-1 == connect(sockfd, res->ai_addr, sizeof(*(res->ai_addr)))) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	while (1) {

		/* get line from user ... */
		if (!fgets(buf, MAXLINE, stdin))
			break;

		n = strlen(buf);

		/* Send output */
		if ( (n = send(sockfd, buf, n, 0)) < 0) {
			perror("send");
			exit(EXIT_FAILURE);
		}

		/* Receive response */
		if ( (n = recv(sockfd, buf, MAXLINE, 0)) < 0) {
			perror("recv");
			exit(EXIT_FAILURE);
		}

		/* make sure it is terminated */
		buf[n] = '\0';

		fputs(buf, stdout);
	}

	return EXIT_SUCCESS;
}
