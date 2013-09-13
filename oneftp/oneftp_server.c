#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
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
	char *file;
	char *port;
	int outfd;
	int n;
	struct addrinfo hints;
	struct addrinfo *res;
	int sockfd;
	int act_sockfd;
	uint8_t buf[MAXBUF];
	int err = 0;

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
		perr_quit(err_null, "unable to open file");
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;

	if ( (n = getaddrinfo(NULL, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
		err = 1;
		goto err_file;
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (-1 == sockfd) {
		perr_quit(err_getaddrinfo, "socket");
	}

	n = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if (-1 == n) {
		perr_quit(err_sock, "bind");
	}

	// listen for a single connection
	n = listen(sockfd, 0);
	if (-1 == n) {
		perr_quit(err_sock, "listen");
	}

	// only accept once
	act_sockfd = accept(sockfd, res->ai_addr, &(res->ai_addrlen));
	if (-1 == act_sockfd) {
		perr_quit(err_sock, "accept");
	}

	while ( (n = recv(act_sockfd, buf, sizeof(buf), 0))) {
		if (-1 == n) {
			if (EINTR == errno)
				continue;
			perr_quit(err_act_sock, "recv");
		}

		write(outfd, buf, n);
	}

err_act_sock:
	close(act_sockfd);
err_sock:
	close(sockfd);
err_getaddrinfo:
	freeaddrinfo(res);
err_file:
	close(outfd);
err_null:

	return err;
}
