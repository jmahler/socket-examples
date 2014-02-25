#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXBUF 4096

struct addrinfo *res = NULL;
int outfd = 0;
int act_sockfd = 0;
int sockfd = 0;

void cleanup() {
	if (act_sockfd) {
		close(act_sockfd);
	}

	if (sockfd) {
		close(sockfd);
	}

	if (res) {
		freeaddrinfo(res);
	}

	if (outfd) {
		close(outfd);
	}
}

#define cleanup_exit()	\
	cleanup();			\
	exit(EXIT_FAILURE);

int main(int argc, char* argv[]) {
	char *file;
	char *port;
	int n;
	struct addrinfo hints;
	uint8_t buf[MAXBUF];

	if (argc != 2) {
		fprintf(stderr, "usage: %s <out file>\n", argv[0]);
		return 1;
	}
	file = argv[1];
	port = "5001";

	outfd = open(file, O_WRONLY | O_CREAT | O_TRUNC,
								  S_IRUSR | S_IWUSR
								| S_IRGRP | S_IWGRP
								| S_IROTH);
	if (-1 == outfd) {
		perror("unable to open file");
		cleanup_exit();
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;

	if ( (n = getaddrinfo(NULL, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
		cleanup_exit();
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (-1 == sockfd) {
		perror("socket");
		cleanup_exit();
	}

	n = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if (-1 == n) {
		perror("bind");
		cleanup_exit();
	}

	/* listen for a single connection */
	n = listen(sockfd, 0);
	if (-1 == n) {
		perror("listen");
		cleanup_exit();
	}

	/* only accept once */
	act_sockfd = accept(sockfd, res->ai_addr, &(res->ai_addrlen));
	if (-1 == act_sockfd) {
		perror("accept");
		cleanup_exit();
	}

	while ( (n = recv(act_sockfd, buf, sizeof(buf), 0))) {
		if (-1 == n) {
			if (EINTR == errno)
				continue;
			perror("recv");
			cleanup_exit();
		}

		write(outfd, buf, n);
	}

	cleanup();

	return 0;
}
