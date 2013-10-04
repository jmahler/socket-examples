/*
 * snw-client.c (derived from echo_client.c)
 *
 * This is a echo client except that it sends a file to the echo server
 * and then stores the result.
 *
 * This uses the unreliable send to (packetErrorSendTo).
 * If it takes longer than TIMEOUT_MS to receive a reply from the
 * echo server it will timeout and resend the message.
 *
 *   (terminal 1)
 *   ./snw-server
 *   Port: 16245
 *
 *   (terminal 2)
 *   ./snw-client localhost 16245 data
 *   (output will be in data.out)
 *
 *   md5sum data.*
 *   (should be identical)
 *
 * To generate test data the 'dd' command can be used.
 *
 *   dd if=/dev/urandom of=data bs=1M count=10
 *
 * Author:
 *   Jeremiah Mahler <jmmahler@gmail.com>
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

#define DEBUG 1

// Transfer will be attempted at MAXLINE,
//   partial transfers are handled for any size.
//#define MAXLINE 1000	// too small
#define MAXLINE 1300	// packetErrorSendTo maximum, fastest
//#define MAXLINE 1500	// MTU
//#define MAXLINE 2500	// too large

// If the timeout is too short duplicates will be sent and
// errors will result.  So this time must be larger than the
// worst case error.  The time provided by ping can be helpful.
//#define TIMEOUT_MS 0.2  // errors on localhost
//#define TIMEOUT_MS 0.4  // OK on localhost
#define TIMEOUT_MS 400  // OK on the internet, twic ~200 ms ping time

int sockfd;
struct addrinfo *res;

void cleanup(void) {
	if (sockfd > 0)
		close(sockfd);

	if (res)
		freeaddrinfo(res);
}

int main(int argc, char* argv[]) {
	char *infile;
	char outfile[1024];
	int infd;
	int outfd;

	struct addrinfo hints;
	int n;
	char *host;
	char *port;

	char sbuf[MAXLINE];  // send buffer
	char rbuf[MAXLINE];  // receive buffer

	struct timeval tv;
	fd_set rd_set;
	int to_send, sent, recvd, to_write;
	int si, wi;

#if DEBUG
	int timeout_count = 0;
	int sent_bytes = 0;
	int recvd_bytes = 0;
	int num_send = 0;
	int num_recv = 0;
#endif

	// configure at exit cleanup function
	sockfd = 0;
	res = NULL;
	if (atexit(cleanup) != 0) {
		fprintf(stderr, "unable to set exit function\n");
		exit(EXIT_FAILURE);
	}
	signal(SIGINT, exit);  // catch Ctrl-C/Ctrl-D and exit

	if (argc != 4) {
		fprintf(stderr, "usage: %s <host> <port> <input file>\n", argv[0]);
		fprintf(stderr, "                output -> <in file>.out\n");
		exit(EXIT_FAILURE);
	}
	host = argv[1];
	port = argv[2];
	infile = argv[3];

	infd = open(infile, O_RDONLY);
	if (-1 == infd) {
		perror("open infile");
		exit(EXIT_FAILURE);
	}

	n = snprintf(outfile, sizeof(outfile), "%s.out", infile);
	if (n < 0) {
		fprintf(stderr, "snprintf failed\n");
		exit(EXIT_FAILURE);
	}

	outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC,
								  S_IRUSR | S_IWUSR
								| S_IRGRP | S_IWGRP
								| S_IROTH);
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

	while ( (n = read(infd, sbuf, MAXLINE))) {
		if (-1 == n) {
			if (errno == EINTR)
				continue;

			perror("read");
			exit(EXIT_FAILURE);
		}

		to_send = n;
		si = 0;
		while (to_send) {
#if DEBUG
			num_send++;
			printf("sendto(%d)\n", to_send);
#endif
			n = packetErrorSendTo(sockfd, sbuf+si, to_send, 0,
									res->ai_addr, res->ai_addrlen);
			if (-1 == n) {
				if (errno == EINTR)
					continue;

				perror("sendto");
				exit(EXIT_FAILURE);
			}
			sent = n;

			FD_ZERO(&rd_set);
			FD_SET(sockfd, &rd_set);

			tv.tv_sec = 0;
			tv.tv_usec = TIMEOUT_MS*1000;

			n = select(sockfd+1, &rd_set, NULL, NULL, &tv);
			if (-1 == n) {
				perror("select()");
				exit(EXIT_FAILURE);
			} else if (0 == n) {
				// time out
#if DEBUG
				timeout_count++;
				printf("timeout\n");
#endif
				continue;
			}

			// if we can read something,
			//   our send was a success
			to_send -= sent;
			si += sent;
#if DEBUG
			sent_bytes += sent;
#endif

			while (1) {
				n = recvfrom(sockfd, rbuf, sent, 0, NULL, NULL);
#if DEBUG
				num_recv++;
#endif
				if (-1 == n) {
					if (errno == EINTR)
						continue;

					perror("recvfrom");
					exit(EXIT_FAILURE);
				}
				recvd = n;
#if DEBUG
				printf("recvfrom(%d)\n", recvd);
#endif

				if (recvd != sent) {
					fprintf(stderr, "sent(%d) != recvd(%d), ", sent, recvd);
					fprintf(stderr, "  server is mis-behaving.\n");
				}

#if DEBUG
				recvd_bytes += recvd;
#endif

				to_write = recvd;
				wi = 0;
				while (to_write) {
					n = write(outfd, rbuf+wi, n);
					if (-1 == n) {
						if (errno == EINTR)
							continue;

						perror("write");
						exit(EXIT_FAILURE);
					}

					to_write -= n;
					wi += n;
				}

				break;
			}
		}
	}

#if DEBUG
	printf("  sent (bytes): %d\n", sent_bytes);
	printf(" recvd (bytes): %d\n", recvd_bytes);
	printf("       # sends: %d\n", num_send);
	printf("       # recvs: %d\n", num_recv);
	printf("  repeat sends: %d (%.2f%%)\n", timeout_count,
					((double) timeout_count / (double) num_send)*100);
#endif

	return EXIT_SUCCESS;
}
