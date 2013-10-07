/*
 * snw-client.c
 *
 * This is a echo client except that it sends a file to a echo server
 * instead of taking input from stdin.  The data echoed back is then
 * stored to a file (.out).
 *
 * This uses the unreliable sendto function (packetErrorSendTo).
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
 * To generate data the 'dd' command can be used.
 * The following generates 10M of random data.
 *
 *   dd if=/dev/urandom of=data bs=1M count=10
 *
 * To enable all sorts of debugging output and statistics set DEBUG=1
 * and recompile.
 *
 * The maximum sequence number (MAXSEQ) and timeout (TIMEOUT_MS)
 * can also be changed to see the effects.
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

#define DEBUG 0

// MAXLINE determines the buffer size for send/recv.
// This code will handle any size but it is fastest
// when it is the same size as the sendto function.
//
//#define MAXLINE 1000	// too small
#define MAXLINE 1300	// packetErrorSendTo maximum, fastest
//#define MAXLINE 1500	// MTU
//#define MAXLINE 2500	// too large

// maximum sequence number
#define MAXSEQ 1	// 0, 1
//#define MAXSEQ 2	// 0, 1, 2

// TIMEOUT_MS determines the recvfrom timeout before a packet
// is re-sent.  With two sequence numbers (MAXSEQ=1) it can
// detect duplicates and handle a very small time out (0.001 ms).
// With one sequence number (MAXSEQ=0) duplicates cannot be detected
// and the delay has to be very large (> 400 ms).
//
// If the delay is too small it will be slow because it has to deal with
// many duplicates.  If it is too long it will be slow due to the number
// of timeouts.  There is an optimal value somewhere in between depending
// upon the the latency of the network being used.  In any case it should
// handle duplicate packets properly and avoid any data corruption.
//
//#define TIMEOUT_MS 0.1  // localhost
#define TIMEOUT_MS 1
//#define TIMEOUT_MS 100  // slow remote host

int sockfd;
struct addrinfo *res;

void cleanup(void) {
	if (sockfd > 0)
		close(sockfd);

	if (res)
		freeaddrinfo(res);
}

#define HEADER_SZ	sizeof(unsigned char)
#define BUF_SZ		MAXLINE - HEADER_SZ
// An entire packet must not exceed MAXLINE bytes.
struct arq_packet {
	// header
	unsigned char seq;
	// data
	char buf[BUF_SZ];
};

int main(int argc, char* argv[]) {
	char *infile;
	char outfile[1024];
	int infd;
	int outfd;

	int n;

	struct addrinfo hints;
	char *host;
	char *port;

	struct arq_packet sbuf;	// send buffer
	struct arq_packet rbuf;	// receive buffer

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
	int invalid_seq_count = 0;
#endif

	// Setup the cleanup() function to be called at exit() or Ctrl-C.
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

	// The output file is the infile with .out appended.
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

	sbuf.seq = 0;  // initial sequence number

	while ( (n = read(infd, (void *) &sbuf.buf, BUF_SZ))) {
		if (-1 == n) {
			if (errno == EINTR)
				continue;

			perror("read");
			exit(EXIT_FAILURE);
		}

		// The amount read from a file may be too large to send in one
		// operation.  It will loop until there is no data left to send.
		to_send = HEADER_SZ + n;
		si = 0;
		while (to_send) {
#if DEBUG
			num_send++;
			printf("sendto(bytes=%d, seq=%d)\n", to_send, sbuf.seq);
#endif

			// Send some data then setup a timeout waiting to read.
			// If it times out, automatically repeat the request (ARQ).

			n = packetErrorSendTo(sockfd, &sbuf + si, to_send, 0,
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

#if DEBUG
			sent_bytes += sent;
#endif

			// We can read something but it my be a duplicate...

			while (1) {
				// It is assumed that the amount received will be identical
				// to the amount sent.  This is checked below (recvd != send).
				n = recvfrom(sockfd, (void *) &rbuf, sent, 0, NULL, NULL);
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
				printf("recvfrom(bytes=%d, seq=%d)\n", recvd, rbuf.seq);
#endif
				break;
			}

			if (recvd != sent) {
				fprintf(stderr, "sent(%d) != recvd(%d), ", sent, recvd);
				fprintf(stderr, "  server is mis-behaving.\n");
				exit(EXIT_FAILURE);
			}

			if (rbuf.seq != sbuf.seq) {
				// An unexpected sequence number was received.
#if DEBUG
				printf("INVALID SEQUENCE NUMBER!\n");
				invalid_seq_count++;
#endif
				// discard this packet

				// continue to re-send the packet
				continue;
			}
#if DEBUG
			recvd_bytes += recvd;
#endif

			// A successful write and read was completed!
			//
			// Steps left:
			//
			//   - write the data to the .out file
			//   - go to the next chunk of data (to_send, si).
			//   - update the sequence number
			//

			// write the data to the .out file
			to_write = recvd - HEADER_SZ;
			wi = 0;
			while (to_write) {
				n = write(outfd, rbuf.buf + wi, to_write);
				if (-1 == n) {
					if (errno == EINTR)
						continue;

					perror("write");
					exit(EXIT_FAILURE);
				}
#if DEBUG
				printf("wrote to file %i B\n", to_write);
#endif
				to_write -= n;
				wi += n;
			}

			// go to the next chunk of data
			to_send -= sent;
			si += sent;

			// update the sequence number
			if (++sbuf.seq > MAXSEQ) {
				sbuf.seq = 0;
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
	printf("   invalid seq: %d\n", invalid_seq_count);
#endif

	return EXIT_SUCCESS;
}
