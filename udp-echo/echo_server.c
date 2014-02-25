
/*
 * echo_server.c
 *
 * Echo server for use with the echo client.
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

#define PORT "16245"
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

	struct addrinfo hints, *p;
	int n;
	int sockfd;
	uint8_t msg[MAXLINE];
	struct sockaddr cliaddr;
	socklen_t cliaddr_len;
	int left;
	int i;

	sockfd = 0;
	res = NULL;
	if (atexit(cleanup) != 0) {
		fprintf(stderr, "unable to set exit function\n");
		exit(EXIT_FAILURE);
	}
	signal(SIGINT, exit);  /* catch Ctrl-C/Ctrl-D and exit */

	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_INET;
	hints.ai_socktype 	= SOCK_DGRAM;
	hints.ai_flags 		= AI_PASSIVE;

	if ( (n = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
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

		printf("Port: %s\n", PORT);

		break;
	}
	if (NULL == p) {
		fprintf(stderr, "unable to bind");
		exit(EXIT_FAILURE);
	}

	while (1) {
		cliaddr_len = sizeof(cliaddr);

		n = recvfrom(sockfd, msg, MAXLINE, 0, &cliaddr, &cliaddr_len);
		if (-1 == n) {
			if (errno == EINTR)
				continue;

			perror("recvfrom");
			exit(EXIT_FAILURE);
		}

		left = n;
		i = 0;
		while (left) {
			n = sendto(sockfd, msg+i, left, 0, &cliaddr, cliaddr_len);
			if (-1 == n) {
				if (errno == EINTR)
					continue;

				perror("sendto");
				exit(EXIT_FAILURE);
			}
			i += n;
			left -= n;
		}
	}

	return EXIT_SUCCESS;
}
