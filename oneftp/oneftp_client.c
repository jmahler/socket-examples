#include <sys/socket.h>
#include <sys/stat.h>
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
int sockfd = 0;
int infd = 0;

void cleanup() {

	if (sockfd) {
		close(sockfd);
	}

	if (res) {
		freeaddrinfo(res);
	}

	if (infd) {
		close(infd);
	}
}

#define cleanup_exit()	\
	cleanup();			\
	exit(EXIT_FAILURE);

int main(int argc, char* argv[]) {
	char *server_name;
	char *file;
	char *port;
	struct addrinfo hints;
	int n;
	ssize_t ret;
	size_t len;
	uint8_t buf[MAXBUF];

	if (argc != 3) {
		fprintf(stderr, "usage: %s <server> <in file>\n", argv[0]);
		return 1;
	}
	server_name = argv[1];
	file 		= argv[2];
	port		= "5001";

	infd = open(file, O_RDONLY);
	if (-1 == infd) {
		perror("unable to open file");
		cleanup_exit();
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_INET;
	hints.ai_socktype 	= SOCK_STREAM;

	if ( (n = getaddrinfo(server_name, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
		cleanup_exit();
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (-1 == sockfd) {
		perror("socket");
		cleanup_exit();
	}

	if (-1 == connect(sockfd, res->ai_addr, sizeof(*(res->ai_addr)))) {
		perror("connect");
		cleanup_exit();
	}

	while ( (ret = read(infd, buf, MAXBUF))) {
		if (-1 == ret) {
			if (errno == EINTR)
				continue;
			perror("read");
			cleanup_exit();
		}
		len = ret;

		while (len) {
			ret = send(sockfd, buf, len, 0);
			if (-1 == ret) {
				if (errno == EINTR)
					continue;
				perror("send");
				cleanup_exit();
			}
			len -= ret;
		}
	}

	cleanup();

	return 0;
}
