/*
 * A function that randomly drops a packet. packetErrorSendTo has the same
 * format and arguments as sendto(2).
 */
#include <sys/types.h>
#include <sys/socket.h>

#define MAX_PACKET_DATA_SIZE (1300)

ssize_t packetErrorSendTo(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
