
#include <sys/types.h>
#include <sys/socket.h>

#define MAX_PACKET_DATA_SIZE (1300)

/* Works just like sendto(2) except it is grossly unreliable and
 * randomly drops packets.
 */
ssize_t unreliable_sendto(int sockfd, const void *buf, size_t len, int flags,
		const struct sockaddr *dest_addr, socklen_t addrlen);
