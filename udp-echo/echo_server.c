
/*
 * echo_server.c
 *
 * Datagram (UDP) echo server for use with the echo client.
 *
 *   (terminal 1)
 *   ./echo_server
 *   Port: 16245
 *
 *   (terminal 2)
 *   ./echo_client localhost 16245
 *
 * Refer to echo_client.c for information on its usage.
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

/*#define PORT "16245"*/	/* > 0, set to specific port */
#define PORT "0"		/* allocate a free port */

#define MAXLINE 1500

static int quit = 0;
void int_handler() {
	quit = 1;
}

int main(int argc, char* argv[]) {

	struct addrinfo hints, *p;
	int n;
	uint8_t msg[MAXLINE];
	struct sockaddr cliaddr;
	socklen_t cliaddr_len;
	struct sockaddr_in sin;
	socklen_t len;
	int sockfd = 0;
	struct addrinfo *res = NULL;
	struct sigaction int_act;

	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	memset(&int_act, 0, sizeof(int_act));
	int_act.sa_handler = int_handler;
	sigaction(SIGINT, &int_act, 0);  /* catch Ctrl-C/Ctrl-D */

	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_INET;
	hints.ai_socktype 	= SOCK_DGRAM;
	hints.ai_flags 		= AI_PASSIVE;

	if ( (n = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
		exit(EXIT_FAILURE);
	}

	/* loop through addresses, use first one that works */
	for (p = res; p != NULL; p = p->ai_next) {
		/* Create the socket */
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
							p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

		/* Bind the socket */
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);	
			perror("bind");
			continue;
		}

		break;
	}
	if (NULL == p) {
		fprintf(stderr, "No address found.");
		exit(EXIT_FAILURE);
	}

	/* find the port number we were given */
	if (p->ai_family == AF_INET) {
		len = sizeof(struct sockaddr_in);
		if (getsockname(sockfd, (struct sockaddr *) &sin, &len) == -1) {
			perror("getsockname");
			exit(EXIT_FAILURE);
		}

		printf("Port: %u\n", ntohs(sin.sin_port));
	} else {
		fprintf(stderr, "only ipv4 supported\n");
		exit(EXIT_FAILURE);
	}

   	/* Receive input, echo back */
	while (!quit) {
		cliaddr_len = sizeof(cliaddr);

		n = recvfrom(sockfd, msg, MAXLINE, 0, &cliaddr, &cliaddr_len);
		if (n < 0) {
			perror("recvfrom");
			exit(EXIT_FAILURE);
		}

		n = sendto(sockfd, msg, n, 0, &cliaddr, cliaddr_len);
		if (n < 0) {

			perror("sendto");
			exit(EXIT_FAILURE);
		}
	}

	if (sockfd > 0)
		close(sockfd);

	if (res)
		freeaddrinfo(res);

	return EXIT_SUCCESS;
}
