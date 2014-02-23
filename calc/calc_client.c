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
#include <stdlib.h>
#include <unistd.h>

#define MAXLINE 2000  // maximum number of input characters
#define MAXRECV 20	  // maximum size of recieve buffer (char)

int sockfd = 0;
struct addrinfo *srvinfo = NULL;

void cleanup() {

	if (sockfd) {
		close(sockfd);
	}
		
	if (srvinfo) {
		freeaddrinfo(srvinfo);
	}
}

#define cleanup_exit()	\
	cleanup();			\
	exit(EXIT_FAILURE);

int main(int argc, char* argv[]) {
	struct addrinfo hints;
	int n;
	size_t sz;
	char *server_name;
	char *port;
	char line[MAXLINE+2];  // +2 for '\n\0'
	char recv_line[MAXRECV];
	ssize_t ret;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <server> <port>\n", argv[0]);
		return 1;
	}
	server_name = argv[1];
	port		= argv[2];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(server_name, port, &hints, &srvinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
		cleanup_exit();
	}

	sockfd = socket(srvinfo->ai_family, srvinfo->ai_socktype, srvinfo->ai_protocol);
	if (-1 == sockfd) {
		perror("Unable to create socket.");
		cleanup_exit();
	}

	if (-1 == connect(sockfd, srvinfo->ai_addr, sizeof(*(srvinfo->ai_addr)))) {
		perror("connect");
		cleanup_exit();
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
			cleanup_exit();
		}

		ret = recv(sockfd, recv_line, MAXRECV, 0);
		if (-1 == ret) {
			perror("recv");
			cleanup_exit();
		}

		// enforce null line termination
		recv_line[ret] = '\0';

		printf("Answer: %s\n", recv_line);
	}

	cleanup();

	return 0;
}
