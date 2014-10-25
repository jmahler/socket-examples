
/*
 * echo_client.c
 *
 * Unix domain socket echo client for use with an echo server.
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
	int sockfd;
	struct sockaddr_un addr;
	char buf[MAXLINE];
	int n;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <socket path>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (-1 == sockfd) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_LOCAL;
	strncpy(addr.sun_path, argv[1], sizeof(addr.sun_path) - 1);

	if (-1 == connect(sockfd, (struct sockaddr *) &addr, sizeof(addr))) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	while (1) {

		if (!fgets(buf, MAXLINE, stdin))
			break;
		n = strlen(buf);

		if ( (n = send(sockfd, buf, n, 0)) < 0) {
			perror("send");
			exit(EXIT_FAILURE);
		}

		if ( (n = recv(sockfd, buf, MAXLINE - 1, 0)) < 0) {
			perror("recv");
			exit(EXIT_FAILURE);
		}
		buf[n] = '\0';

		fputs(buf, stdout);
	}

	return EXIT_SUCCESS;
}
