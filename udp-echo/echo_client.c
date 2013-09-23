
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAXLINE 1500

int main(int argc, char* argv[]) {
	int err = 0;
	struct addrinfo hints, *res;
	int sockfd;
	int n;
	char *host;
	char *port;
	int left, i;
	char buf[MAXLINE];

	if (argc != 3) {
		fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
		err = 1;
		goto err_usage;
	}
	host = argv[1];
	port = argv[2];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_INET;
	hints.ai_socktype 	= SOCK_DGRAM;

	if ( (n = getaddrinfo(host, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
		err = 1;
		goto err_addrinfo;
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("socket");
		err = 1;
		goto err_socket;
	}

	while (fgets(buf, MAXLINE, stdin)) {
		n = strlen(buf);

		// write the line to the server
		left = n;
		i = 0;
		while (left) {
			n = sendto(sockfd, buf+i, left, 0, res->ai_addr, res->ai_addrlen);
			if (-1 == n) {
				if (errno == EINTR)
					continue;

				perror("sendto");
				err = 1;
				goto err_sendto;
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
				err = 1;
				goto err_recvfrom;
			}

			// make sure it is terminated
			buf[n] = '\0';

			fputs(buf, stdout);

			break;
		}
	}

err_recvfrom:
err_sendto:
	close(sockfd);
err_socket:
	freeaddrinfo(res);
err_addrinfo:
err_usage:

	return err;
}
