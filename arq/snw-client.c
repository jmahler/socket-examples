/*
 * snw-client.c
 *
 * This client is meant to be used with the snw-server.
 *
 *   (terminal 1)
 *   ./snw-server
 *   Port: 16245
 *
 * It will send the file name that the server should open
 * and read data from.  Then it will read the data and store
 * it to a .out file.
 *
 *   (terminal 2)
 *   ./snw-client localhost 16245 data
 *   (result in data.out)
 *
 * A good way to generate random data is with 'dd'.
 * The following would create 5M of random data.
 *
 *   dd if=/dev/urandom of=data bs=1M count=5
 *
 * Then after the transfer the data received should
 * be the same as what was sent.
 *
 *  md5sum data data.out
 *    fb336fd7cfc098dc6f7fad2d98856210  data
 *    fb336fd7cfc098dc6f7fad2d98856210  data.out
 *
 * Author:
 *
 *   Jeremiah Mahler <jmmahler@gmail.com>
 *
 * Copyright:
 *
 * Copyright &copy; 2015, Jeremiah Mahler.  All Rights Reserved.
 * This project is free software and released under
 * the GNU General Public License.
 *
 */

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "packetErrorSendTo.h"
#include "arq.h"

int quit = 0;
void int_handler() {
	quit = 1;
}

int main(int argc, char* argv[]) {
	char *infile;
	char outfile[1024];
	int outfd;

	int n;

	struct addrinfo hints;
	char *host;
	char *port;

	char sbuf[MAXLINE];	/* send buffer */
	char rbuf[MAXLINE];	/* receive buffer */
	size_t len;

	int sockfd = 0;
	struct addrinfo *res = NULL;

	struct sigaction int_act;

	memset(&int_act, 0, sizeof(int_act));
	int_act.sa_handler = int_handler;
	if (-1 == sigaction(SIGINT, &int_act, 0)) {
		perror("int sigaction failed");
		exit(EXIT_FAILURE);
	}

	if (argc != 4) {
		fprintf(stderr, "usage: %s <host> <port> <input file>\n",
				argv[0]);
		fprintf(stderr, "                output -> <in file>.out\n");
		exit(EXIT_FAILURE);
	}
	host = argv[1];
	port = argv[2];
	infile = argv[3];

	/* The output file is the infile with .out appended. */
	n = snprintf(outfile, sizeof(outfile), "%s.out", infile);
	if (n < 0) {
		fprintf(stderr, "snprintf failed\n");
		exit(EXIT_FAILURE);
	}
	outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	if (-1 == outfd) {
		perror("open outfile");
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_INET;
	hints.ai_socktype 	= SOCK_DGRAM;

	if ( (n = getaddrinfo(host, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
		exit(EXIT_FAILURE);
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/* send the file name */
	len = (strlen(infile) + 1)*sizeof(*infile);
	memcpy(&sbuf, infile, len);
	n = arq_sendto(sockfd, &sbuf, len, 0, res->ai_addr, res->ai_addrlen);
	if (-1 == n) {
		perror("arq_sendto");
		exit(EXIT_FAILURE);
	}

	while ( (n = arq_recvfrom(sockfd, &rbuf, MAXLINE, 0, NULL, NULL))) {
		if (quit)
			break;

		if (-1 == n) {
			perror("arq_recvfrom");
			exit(EXIT_FAILURE);
		}

		n = write(outfd, rbuf, n);
		if (-1 == n) {
			perror("write");
			exit(EXIT_FAILURE);
		}
	}

	if (sockfd > 0)
		close(sockfd);

	if (res)
		freeaddrinfo(res);

	return EXIT_SUCCESS;
}
