/*
 * snw-server.c
 *
 * On startup this server will bind to a free port and
 * display which one it is using.
 *
 *   (terminal 1)
 *   ./snw-server
 *   Port: 16245
 *
 * Then the snw-client can connect and send a string of
 * the file name that the server should open and read
 * data from.  The server will transfer the data from
 * this file and exit.
 *
 *   (terminal 2)
 *   ./snw-client localhost 16245 data
 *
 * Author:
 *
 *   Jeremiah Mahler <jmmahler@gmail.com>
 *
 * Copyright:
 *
 * Copyright &copy; 2014, Jeremiah Mahler.  All Rights Reserved.
 * This project is free software and released under
 * the GNU General Public License.
 *
 */

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arq.h"

int quit = 0;
void int_handler() {
	quit = 1;
}

int main(int argc, char* argv[]) {

	struct addrinfo hints, *p;

	int n;

	struct sockaddr cliaddr;
	socklen_t cliaddr_len;
	struct sockaddr_in sin;
	socklen_t len;

	char sbuf[MAXDATA];
	char rbuf[MAXDATA];

	char *infile;
	int infd;

	size_t left;
	size_t i;

	int sockfd = 0;
	struct addrinfo *res = NULL;

	struct sigaction int_act;

	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* catch Ctrl-C/Ctrl-D and exit */
	memset(&int_act, 0, sizeof(int_act));
	int_act.sa_handler = int_handler;
	if (-1 == sigaction(SIGINT, &int_act, 0)) {
		perror("int sigaction failed");
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_INET;
	hints.ai_socktype 	= SOCK_DGRAM;
	hints.ai_flags 		= AI_PASSIVE;

	if ( (n = getaddrinfo(NULL, "0", &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
		exit(EXIT_FAILURE);
	}

	for (p = res; p != NULL; p = p->ai_next) {

		if ((sockfd = socket(p->ai_family, p->ai_socktype,
							p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("bind");
			continue;
		}

		/* find the port number we were given */
		if (p->ai_family == AF_INET) {
			len = sizeof(struct sockaddr_in);
			if (getsockname(sockfd, (struct sockaddr *) &sin, &len) == -1) {
				close(sockfd);	
				perror("getsockname");
				continue;
			}

			printf("Port: %u\n", ntohs(sin.sin_port));
		} else {
			fprintf(stderr, "only ipv4 supported\n");
			close(sockfd);
			continue;
		}

		break;
	}
	if (NULL == p) {
		fprintf(stderr, "unable to bind\n");
		exit(EXIT_FAILURE);
	}

	/* receive the file name */
	while (1) {
		cliaddr_len = sizeof(cliaddr);
		n = arq_recvfrom(sockfd, rbuf, MAXDATA, 0,
										&cliaddr, &cliaddr_len);
		if (-1 == n) {
			perror("arq_recvfrom");
			exit(EXIT_FAILURE);
		}
		break;
	}

	infile = rbuf;
	infd = open(infile, O_RDONLY);
	if (-1 == infd) {
		perror("open infile");
		exit(EXIT_FAILURE);
	}

	while ( (n = read(infd, &sbuf, MAXDATA))) {
		if (quit)
			break;

		if (-1 == n) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		left = n;
		i = 0;
		while (left) {
			n = arq_sendto(sockfd, &sbuf + i, left, 0, &cliaddr, cliaddr_len);
			if (-1 == n) {
				perror("arq_sendto");
				exit(EXIT_FAILURE);
			}
			left -= n;
			i += n;
		}
	}

	/* An empty send signals EOF */
	n = arq_sendto(sockfd, &sbuf, 0, 0, &cliaddr, cliaddr_len);
	if (-1 == n) {
		perror("arq_sendto, EOF");
		exit(EXIT_FAILURE);
	}

	if (sockfd > 0)
		close(sockfd);

	if (res)
		freeaddrinfo(res);

	return EXIT_SUCCESS;
}
