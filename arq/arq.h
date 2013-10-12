/*
 * arq.h
 *
 * Given a grossly unreliable sendto() function (packetErrorSendTo)
 * the functions provided here construct a reliable layer using
 * an Automatic Repeat Request (ARQ) scheme.
 *
 * The ARQ scheme uses a 1 bit sequence number and no sliding window.
 * Every send must get an ACK with the expected sequence number otherwise
 * it will be re-sent.
 *
 * The timeout between re-sends without an ACK can be configured using
 * the TIMEOUT_MS define.  There is some optimal value which is dependent
 * upon the network being used.  If it is too small too many resends
 * will be made which will have to be handled causing it to become slow.
 * If it is too large it timeout will make it slow.
 * Note that it should still be reliable even with many resends.
 *
 * Author:
 *
 *   Jeremiah Mahler <jmmahler@gmail.com>
 *
 * Copyright:
 *
 * Copyright &copy; 2013, Jeremiah Mahler.  All Rights Reserved.
 * This project is free software and released under
 * the GNU General Public License.
 */

#ifndef _ARQ_H
#define _ARQ_H

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "packetErrorSendTo.h"

#define TIMEOUT_MS 0.5

#define MAXLINE 1300
#define HEADER_SZ	2
#define ACK_SZ		HEADER_SZ
#define DATA_SZ		MAXLINE - HEADER_SZ
#define MAXDATA		DATA_SZ

struct arq_packet {
	// header
	unsigned char type;
	unsigned char seq;
	// data
	char data[DATA_SZ];
};

enum {
	TYPE_ACK,
	TYPE_DATA
};

#define MIN(a, b) ( ((a) < (b)) ? (a) : (b))

extern int seq;
extern int recv_seq;

#define MAX_RESEND 3

/*
 * arq_sendto()
 *
 * Works the same as sendto() except that it uses the
 * unreliable packetErrorSendTo function and it uses ARQ
 * methods to make it reliable again.
 *
 * In the event that the other end quits responding it will
 * give up try to resend after MAX_RESEND times.
 * And it will return 0 indicating that no data was sent.
 *
 */
int arq_sendto(int sockfd, void *buf, size_t len,
			int flags, const struct sockaddr *dest_addr, socklen_t addrlen);

/*
 * arq_recvfrom()
 *
 * Works identically to recvfrom() except that it will return an
 * ACK for each data packet according to the ARQ scheme used here.
 */
int arq_recvfrom(int sockfd, void *buf, size_t len,
		int flags, struct sockaddr *src_addr, socklen_t *addrlen);

#endif // _ARQ_H
