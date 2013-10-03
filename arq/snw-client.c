
/*
 * snw-client.c (derived from echo_client.c)
 *
 * An echo client that uses the unreliable send to (packetErrorSendTo).
 * It it takes longer than TIMEOUT_MS to receive a reply from the
 * echo server it will timeout and resend the message.
 *
 *   (terminal 1)
 *   ./snw-server
 *   Port: 16245
 *
 *   (terminal 2)
 *   ./snw-client localhost 16245
 *   (type in characters and the result will be echoed back)
 *
 * Author:
 *   Jeremiah Mahler <jmmahler@gmail.com>
 *
 */

#include <sys/select.h>
#include <sys/time.h>
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
#define TIMEOUT_MS 100
#define DEBUG 0

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
	char sbuf[MAXLINE];  // send buffer
	char rbuf[MAXLINE];  // receive buffer
	int len;
	struct timeval tv;
	fd_set rd_set;

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

	FD_ZERO(&rd_set);
	FD_SET(sockfd, &rd_set);

	while (fgets(sbuf, MAXLINE, stdin)) {
		len = strlen(sbuf);

		// sendto with ARQ
		while (1) {
			// write the line to the server
			left = len;
			i = 0;
			while (left) {
				n = packetErrorSendTo(sockfd, sbuf+i, left, 0,
										res->ai_addr, res->ai_addrlen);
				if (-1 == n) {
					if (errno == EINTR)
						continue;

					perror("sendto");
					exit(EXIT_FAILURE);
				}
				left -= n;
				i += n;
			}

			tv.tv_sec = 0;
			tv.tv_usec = TIMEOUT_MS*1000;

			n = select(sockfd+1, &rd_set, NULL, NULL, &tv);
			if (-1 == n) {
				perror("select()");
				exit(EXIT_FAILURE);
			} else if (0 == n) {
				// time out
				if (DEBUG) printf("timeout\n");
				continue;
			}
			// can read something
			break;
		}

		// read the result from the server, print to stdout
		while (1) {
			n = recvfrom(sockfd, rbuf, MAXLINE, 0, NULL, NULL);
			if (-1 == n) {
				if (errno == EINTR)
					continue;

				perror("recvfrom");
				exit(EXIT_FAILURE);
			}

			// make sure it is terminated
			rbuf[n] = '\0';

			fputs(rbuf, stdout);

			break;
		}
	}

	return EXIT_SUCCESS;
}
