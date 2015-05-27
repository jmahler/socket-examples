
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
 *      |         data         |
 *      |<---------------------+
 *      |                      |
 *      |          ACK         |
 *      +------------>X        |
 *      |                      |
 *      |                  (timeout)
 *      |                      |
 *      |         data         |
 *      |<---------------------+
 *      |                      |
 *      |          ACK         |
 *      +--------------------->|
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

int arq_sendto(int sockfd, void *buf, size_t len,
		int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
	int n;

	struct timeval tv;
	fd_set rd_set;

	struct arq_packet ack_buf;
	struct arq_packet sbuf;
	size_t send_len, tot_len, data_len, ack_len;

	unsigned char ack_recvd;

	unsigned char num_resend;

	send_len = HEADER_SZ + MIN(len, DATA_SZ);
	memcpy(&sbuf.data, buf, MIN(len, DATA_SZ));
	sbuf.seq = seq;
	sbuf.type = TYPE_DATA;

	ack_recvd = 0;
	while (!ack_recvd) {
		n = packetErrorSendTo(sockfd, &sbuf, send_len, flags,
				dest_addr, addrlen);
		if (-1 == n) {
			perror("arq_sendto, sendto");
			return -1;
		}

		tot_len = n;
		data_len = tot_len - HEADER_SZ;

		FD_ZERO(&rd_set);
		FD_SET(sockfd, &rd_set);

		tv.tv_sec = 0;
		tv.tv_usec = TIMEOUT_MS*1000;

		n = select(sockfd+1, &rd_set, NULL, NULL, &tv);
		if (-1 == n) {
			perror("select");
			return -1;
		} else if (0 == n) {
			/* timeout */

			/* give up after a maximum number of re-sends */
			if (++num_resend > MAX_RESEND)
				return 0;

			continue;
		}

		n = recvfrom(sockfd, (void *) &ack_buf, ACK_SZ, 0, NULL, NULL);
		if (-1 == n) {
			perror("arq_sendto, recvfrom");
			return -1;
		}

		ack_len = n;

		if (ack_len >= HEADER_SZ && ack_buf.type == TYPE_ACK
				&& ack_buf.seq == seq) {
			/* ACK confirmed */

			seq = (seq) ? 0 : 1;

			ack_recvd = 1;
		}
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
	size_t recv_len, tot_len, data_len;

	unsigned char ack_recvd;

	recv_len = 0;
	data_len = 0;

	ack_recvd = 0;
	while (!ack_recvd) {
		recv_len = HEADER_SZ + MIN(len, DATA_SZ);

		cliaddr_len = sizeof(cliaddr);
		n = recvfrom(sockfd, &rbuf, recv_len, flags,
				&cliaddr, &cliaddr_len);
		if (-1 == n) {
			perror("arq_recvfrom, recvfrom");
			return -1;
		}

		tot_len = n;

		/* assign initial recv_seq number */
		if (-1 == recv_seq) {
			recv_seq = rbuf.seq ? 0 : 1;
		}


		if (tot_len >= HEADER_SZ && rbuf.type == TYPE_DATA
				&& rbuf.seq  != recv_seq) {
			/* received a valid packet */

			/* save the data */
			data_len = tot_len - HEADER_SZ;
			memcpy(buf, rbuf.data, data_len);

			/* next sequence number */
			recv_seq = recv_seq ? 0 : 1;

			ack_recvd = 1;
		}

		/* send an ACK */
		ack_buf.seq = rbuf.seq;
		ack_buf.type = TYPE_ACK;
		n = packetErrorSendTo(sockfd, &ack_buf, ACK_SZ, 0,
				&cliaddr, cliaddr_len);
		if (-1 == n) {
			perror("arq_recvfrom, sendto");
			return -1;
		}
	}

	/* update the receive addresses */
	if (src_addr != NULL)
		memcpy(src_addr, &cliaddr, sizeof(cliaddr));
	if (addrlen != NULL)
		*addrlen = cliaddr_len;

	return data_len;
}
