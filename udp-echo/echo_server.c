
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PORT "16245"
#define MAXLINE 1500

int main(int argc, char* argv[]) {

	int err = 0;
	struct addrinfo hints, *res, *p;
	int n;
	int sockfd;
	uint8_t msg[MAXLINE];
	struct sockaddr cliaddr;
	socklen_t cliaddr_len;
	int left;
	int i;

	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		err = 1;
		goto err_argc;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_INET;
	hints.ai_socktype 	= SOCK_DGRAM;
	hints.ai_flags 		= AI_PASSIVE;

	if ( (n = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
		err = 1;
		goto err_addrinfo;
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
		err = 1;
		goto err_bind;
	}

	while (1) {
		cliaddr_len = sizeof(cliaddr);

		n = recvfrom(sockfd, msg, MAXLINE, 0, &cliaddr, &cliaddr_len);
		if (-1 == n) {
			if (errno == EINTR)
				continue;

			perror("recvfrom");
			err = 1;
			goto err_recvfrom;
		}

		left = n;
		i = 0;
		while (left) {
			n = sendto(sockfd, msg+i, left, 0, &cliaddr, cliaddr_len);
			if (-1 == n) {
				if (errno == EINTR)
					continue;

				perror("sendto");
				err = 1;
				goto err_sendto;
			}
			i += n;
			left -= n;
		}
	}

err_sendto:
err_recvfrom:
	close(sockfd);
err_bind:
	freeaddrinfo(res);
err_addrinfo:
err_argc:

	return err;
}
