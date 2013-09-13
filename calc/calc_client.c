/*
 * calc_client - Calculator client to communicate with calc_server
 *
 * The calculator client provides an interface for interacting with
 * the calculator server.
 *
 *   (terminal 1)
 *   $ ./calc_server 5001
 *
 *   (terminal 2)
 *   $ ./calc_client localhost 5001
 *   Enter expression:1+1
 *   Answer: 2
 *   ...
 *
 * author: Jeremiah Mahler <jmmahler@gmail.com>
 * 
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define MAXLINE 2000  // maximum number of input characters
#define MAXRECV 20	  // maximum size of recieve buffer (char)

int main(int argc, char* argv[]) {
	struct addrinfo hints;
	struct addrinfo *res;
	int n;
	size_t sz;
	char *server_name;
	char *port;
	int sockfd;
	char line[MAXLINE+2];  // +2 for '\n\0'
	char recv_line[MAXRECV];
	ssize_t ret;
	int err = 0;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <server> <port>\n", argv[0]);
		return 1;
	}
	server_name = argv[1];
	port		= argv[2];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(server_name, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
		err = 1;
		goto err_null;
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (-1 == sockfd) {
		perror("Unable to create socket.");
		err = 1;
		goto err_getaddrinfo;
	}

	if (-1 == connect(sockfd, res->ai_addr, sizeof(*(res->ai_addr)))) {
		perror("connect");
		err = 1;
		goto err_sock;
	}

	while (1) {
		printf("Enter expression:");

		// get up to MAXLINE, +1 for '\n', +1 for fgets added '\0'
		if (!fgets(line, MAXLINE+2, stdin))
			continue;

		sz = strlen(line);

		// remove the final new line
		line[--sz] = '\0';

		// quit on an empty string
		if (0 == sz)
			break;
	
		// send the string (including null) to the calc server
		n = send(sockfd, line, sz+1, 0);
		if (-1 == n) {
			perror("send");
			err = 1;
			goto err_sock;
		}

		ret = recv(sockfd, recv_line, MAXRECV, 0);
		if (-1 == ret) {
			perror("recv");
			err = 1;
			goto err_sock;
		}

		// enforce null line termination
		recv_line[ret] = '\0';

		printf("Answer: %s\n", recv_line);
	}

err_sock:
	close(sockfd);
err_getaddrinfo:
	freeaddrinfo(res);
err_null:

	return err;
}
