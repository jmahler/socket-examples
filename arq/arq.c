
#include "arq.h"

/*
 *
 *           Normal Operation
 *           ----------------
 *
 * arq_recvfrom            arq_sendto
 *      |                      |
 *      |         data         |
 *      |<---------------------+
 *      |                      |
 *      |          ACK         |
 *      +--------------------->|
 *      |                      |
 *
 *
 *             Lost Data
 *             ---------
 *
 * arq_recvfrom            arq_sendto
 *      |                      |
 *      |         data         |
 *      |     X<---------------+
 *      |                      |
 *      |                  (timeout)
 *      |                      |
 *      |         data         |
 *      |<---------------------+
 *      |                      |
 *      |          ACK         |
 *      +--------------------->|
 *      |                      |
 *
 *
 *             Lost ACK
 *             --------
 *
 * arq_recvfrom            arq_sendto
 *      |                      |
 *      |         data:0       |
 *      |<---------------------+
 *      |                      |
 *      |          ACK:0       |
 *      +------------>X        |
 *      |                      |
 *      |                  (timeout)
 *      |                      |
 *      |         data:0       |
 *      |<---------------------+      Discard already received data.
 *      |                      |
 *      |          ACK:0       |
 *      +--------------------->|      Re-send the ACK.
 *      |                      |
 *      |                      |
 *
 *             Delayed ACK
 *             -----------
 *
 * arq_recvfrom            arq_sendto
 *      |                      |
 *      |         data:0       |
 *      |<---------------------+      First chunk of data.
 *      |                      |
 *      | ACK:0                |
 *      +--+                   |
 *      |   \              (timeout)
 *      |    \                 |
 *      |     \   data:0       |
 *      |<---------------------+      Resend.
 *      |       \              |
 *      |        +------------>|      Correct sequence number,
 *      |                      |        ACK accepted.
 *      |         data:1       |
 *      |<---------------------+      Next chunk of data.
 *      |                      |
 *      |          ACK:0       |
 *      +--------------------->|      Wrong sequence number,
 *      |                      |        ACK ignored.
 *      |          ACK:1       |
 *      +--------------------->|      Correct sequence number,
 *      |                      |        ACK accepted.
 *      |                      |
 *
 */

int seq = 0;
int recv_seq = -1;

void reset_seq() {
	recv_seq = -1;
}

int arq_sendto(int sockfd, const void *buf, size_t len,
		int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
	int n;

	struct timeval tv;
	fd_set rd_set;

	struct arq_packet ack_buf;
	struct arq_packet sbuf;
	size_t send_len, data_len;

	unsigned char ack_recvd;

	unsigned char num_resend;

	/* build the ARQ packet to be sent */
	send_len = HEADER_SZ + MIN(len, DATA_SZ);
	memcpy(&sbuf.data, buf, MIN(len, DATA_SZ));
	sbuf.seq = seq;
	sbuf.type = TYPE_DATA;
	/* 'send_len' might be less than the requested 'len' */

	/* main loop to send the packet and wait for an ACK */
	ack_recvd = 0;
	num_resend = 0;
	while (!ack_recvd) {

		/* send the ARQ packet */
		n = unreliable_sendto(sockfd, &sbuf, send_len, flags,
				dest_addr, addrlen);
		if (-1 == n) {
			return -1;
		}
		/* 'n' might be less than the requested 'send_len' */

		data_len = n - HEADER_SZ;
		/* Actual amount of data sent minus headers,
		 * used for the return value on success.
		 */

		/* wait for data or a time out */

		FD_ZERO(&rd_set);
		FD_SET(sockfd, &rd_set);

		tv.tv_sec = 0;
		tv.tv_usec = TIMEOUT_MS*1000;

		n = select(sockfd+1, &rd_set, NULL, NULL, &tv);
		if (-1 == n) {
			return -1;
		} else if (0 == n) { /* timeout */

			if (++num_resend > MAX_RESEND)
				return 0;  /* give up */

			continue;  /* retry */
		}

		n = recvfrom(sockfd, (void *) &ack_buf, ACK_SZ, 0, NULL, NULL);
		if (-1 == n) {
			return -1;
		}

		if (n >= HEADER_SZ && ack_buf.type == TYPE_ACK
				&& ack_buf.seq == seq) {
			/* ACK confirmed */

			seq = (seq) ? 0 : 1;

			ack_recvd = 1;
		}
		/* else data is wrong, ignore */
	}

	return data_len;
}

int arq_recvfrom(int sockfd, void *buf, size_t len,
		int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
	int n;
	struct arq_packet ack_buf;

	struct sockaddr cliaddr;
	socklen_t cliaddr_len;

	struct arq_packet rbuf;
	size_t recv_len, data_len;

	unsigned char data_recvd;

	recv_len = 0;
	data_len = 0;

	data_recvd = 0;
	while (!data_recvd) {
		if (src_addr != NULL) {
			memcpy(&cliaddr, src_addr, *addrlen);
			cliaddr_len = *addrlen;
		} else {
			cliaddr_len = sizeof(cliaddr);
		}

		recv_len = HEADER_SZ + MIN(len, DATA_SZ);

		n = recvfrom(sockfd, &rbuf, recv_len, flags,
				&cliaddr, &cliaddr_len);
		if (-1 == n) {
			return -1;
		}

		/* assign initial recv_seq number */
		if (-1 == recv_seq) {
			recv_seq = rbuf.seq ? 0 : 1;
		}

		if (n >= HEADER_SZ && rbuf.type == TYPE_DATA
				&& rbuf.seq != recv_seq) {
			/* received a valid packet */

			/* save the data */
			data_len = n - HEADER_SZ;
			memcpy(buf, rbuf.data, data_len);

			/* next sequence number */
			recv_seq = recv_seq ? 0 : 1;

			data_recvd = 1;
		}

		/* send an ACK */
		ack_buf.seq = rbuf.seq;
		ack_buf.type = TYPE_ACK;
		n = unreliable_sendto(sockfd, &ack_buf, ACK_SZ, 0,
				&cliaddr, cliaddr_len);
		if (-1 == n) {
			return -1;
		}
	}

	/* update the receive addresses */
	if (src_addr != NULL) {
		memcpy(src_addr, &cliaddr, sizeof(cliaddr));
		*addrlen = cliaddr_len;
	}

	return data_len;
}
