
#include "unreliable_sendto.h"

#ifndef UNRELIABLE_SENDTO_H
#define UNRELIABLE_SENDTO_H

static unsigned char rand;

ssize_t unreliable_sendto(int sockfd, const void *buf, size_t len, int flags,
			const struct sockaddr *dest_addr, socklen_t addrlen)
{
	rand++;
	if (rand & 0x4 && rand & 0x2)  /* 1/4 error, burst of 2 */
		return len; /* drop packet, but indicate it was sent */
	else
		return sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

#endif /* UNRELIABLE_SENDTO_H */
