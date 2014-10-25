
/*
 * echo_server.c
 *
 * Unix domain socket echo server for use with an echo client.
 *
 *   (terminal 1)
 *   ./echo_server socket_file
 *
 *   (terminal 2)
 *   ./echo_client socket_file
 *   (type in characters and the result will be echoed back)
 *
 * Author:
 *   Jeremiah Mahler <jmmahler@gmail.com>
 *
 */

#include <linux/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXLINE 1500

int main(int argc, char* argv[])
{
	int sockfd = 0;
	char msg[MAXLINE];
	struct sockaddr_un srvaddr;
	char *socket_path;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <socket path>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	socket_path = argv[1];

	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (-1 == sockfd) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(&srvaddr, 0, sizeof(srvaddr));
	srvaddr.sun_family = AF_LOCAL;
	strncpy(srvaddr.sun_path, socket_path, sizeof(srvaddr.sun_path) - 1);

	/* bind will fail if socket fail is already present */
	unlink(socket_path);

	if (-1 == bind(sockfd, (struct sockaddr *) &srvaddr, sizeof(srvaddr))) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (-1 == listen(sockfd, 1)) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	/* process connections */
	while (1) {
		int connfd;
		struct sockaddr_un cliaddr;
		socklen_t clilen = sizeof(cliaddr);
		int n;

		connfd = accept(sockfd, (struct sockaddr *) &cliaddr, &clilen);
		if (connfd < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		/* echo loop */
		while (1) {
			if ( (n = recv(connfd, msg, MAXLINE, 0)) < 0) {
				perror("recv");
				exit(EXIT_FAILURE);
			} else if (0 == n) {
				/* quit if a blank line is received */
				if (close(connfd) < 0) {
					perror("close(cli_conn)");
					exit(EXIT_FAILURE);
				}
				break;
			}

			if ( (n = send(connfd, msg, n, 0)) < 0) {
				perror("send");
				exit(EXIT_FAILURE);
			}
		}
	}

	return EXIT_SUCCESS;
}
