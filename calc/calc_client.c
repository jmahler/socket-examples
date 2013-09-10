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

#define MAXLINE 2000
#define MAXRECV 20

int main(int argc, char* argv[]) {
	struct addrinfo hints;
	struct addrinfo *res;
	int n;
	size_t sz;
	char *server_name;
	char *port;
	int sockfd;
	char line[MAXLINE+1];
	char recv_line[MAXRECV];

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
		return 1;
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("Unable to create socket.");
		return 1;
	}

	if (-1 == connect(sockfd, res->ai_addr, sizeof(*(res->ai_addr)))) {
		perror("connect");
		return 1;
	}

	while (1) {
		printf("Enter expression:");

		if (!fgets(line, MAXLINE, stdin))
			continue;

		sz = strlen(line);

		// remove the new line
		if (line[sz-1] == '\n')
			line[--sz] = '\0';

		// quit on an empty string
		if (0 == sz)
			break;
	
		// send the string, with null, to the calc server
		send(sockfd, line, sz+1, 0);

		recv(sockfd, recv_line, MAXRECV, 0);
		recv_line[MAXRECV-1] = '\0';
		// TODO - what about a partial recieve

		printf("Answer: %s\n", recv_line);
	}

	close(sockfd);
	freeaddrinfo(res);

	return 0;
}
