
/*
 * echo_client.c
 *
 * Echo client for use with the echo server.
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

#include "packetErrorSendTo.h"

#define MAXLINE 1500

int sockfd;
struct addrinfo *res;

void cleanup(void) {
	if (sockfd > 0)
		close(sockfd);

	if (res)
		freeaddrinfo(res);
}

int main(int argc, char* argv[]) {
	struct addrinfo hints;
	int n;
	char *host;
	char *port;
	int left, i;
	char buf[MAXLINE];

	// configure at exit cleanup function
	sockfd = 0;
	res = NULL;
	if (atexit(cleanup) != 0) {
		fprintf(stderr, "unable to set exit function\n");
		exit(EXIT_FAILURE);
	}
	signal(SIGINT, exit);  // catch Ctrl-C/Ctrl-D and exit


	if (argc != 3) {
		fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	host = argv[1];
	port = argv[2];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_INET;
	hints.ai_socktype 	= SOCK_DGRAM;

	if ( (n = getaddrinfo(host, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
		exit(EXIT_FAILURE);
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	while (fgets(buf, MAXLINE, stdin)) {
		n = strlen(buf);

		// write the line to the server
		left = n;
		i = 0;
		while (left) {
			n = packetErrorSendTo(sockfd, buf+i, left, 0, res->ai_addr, res->ai_addrlen);
			if (-1 == n) {
				if (errno == EINTR)
					continue;

				perror("sendto");
				exit(EXIT_FAILURE);
			}
			left -= n;
			i += n;
		}

		// read the result from the server, print to stdout
		while (1) {
			n = recvfrom(sockfd, buf, MAXLINE, 0, NULL, NULL);
			if (-1 == n) {
				if (errno == EINTR)
					continue;

				perror("recvfrom");
				exit(EXIT_FAILURE);
			}

			// make sure it is terminated
			buf[n] = '\0';

			fputs(buf, stdout);

			break;
		}
	}

	return EXIT_SUCCESS;
}
