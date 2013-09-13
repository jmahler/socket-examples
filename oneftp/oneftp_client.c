#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAXBUF 4096

#define perr_quit(type, msg) \
	perror(msg); \
	err = 1; \
	goto type;

int main(int argc, char* argv[]) {
	char *server_name;
	char *file;
	char *port;
	struct addrinfo hints;
	struct addrinfo *res;
	int n;
	int sockfd;
	int infd;
	ssize_t ret;
	size_t len;
	uint8_t buf[MAXBUF];
	int err = 0;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <server> <in file>\n", argv[0]);
		return 1;
	}
	server_name = argv[1];
	file 		= argv[2];
	port		= "5001";

	infd = open(file, O_RDONLY);
	if (-1 == infd) {
		perr_quit(err_null, "unable to open file");
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_INET;
	hints.ai_socktype 	= SOCK_STREAM;

	if ( (n = getaddrinfo(server_name, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
		err = 1;
		goto err_file;
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (-1 == sockfd) {
		perr_quit(err_getaddrinfo, "socket");
	}

	if (-1 == connect(sockfd, res->ai_addr, sizeof(*(res->ai_addr)))) {
		perr_quit(err_sock, "connect");
	}

	while ( (ret = read(infd, buf, MAXBUF))) {
		if (-1 == ret) {
			if (errno == EINTR)
				continue;
			perr_quit(err_sock, "read");
		}
		len = ret;

		while (len) {
			ret = send(sockfd, buf, len, 0);
			if (-1 == ret) {
				if (errno == EINTR)
					continue;
				perr_quit(err_sock, "send");
			}
			len -= ret;
		}
	}

err_sock:
	close(sockfd);
err_getaddrinfo:
	freeaddrinfo(res);
err_file:
	close(infd);
err_null:

	return err;
}
