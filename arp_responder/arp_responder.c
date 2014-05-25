/*
 * arp_responder.c
 *
 * Given a file of IP address and MAC addresses
 *
 *   192.168.99.44   C0:04:AB:43:22:FF
 *   192.168.99.18   D4:DE:AD:BE:EF:FF
 *
 * this program will respond to any requests for these entries.
 * It, in effect, spoofs these addresses since they don't belong
 * to the current machine.
 *
 *   $ sudo ./arp_responder eth0 addresses.txt
 *   ^C (quit)
 *   $
 *
 * On an wired network if this program is running on machine A,
 * and then one of the entries is pinged by machine B, a complete
 * entry should appear for that address in the arp table (arp -n)
 * of machine B.
 *
 * The libpcap [www.tcpdump.org] library is used to read and send packets.
 *
 * Author:
 *
 *  Jeremiah Mahler <jmmahler@gmail.com>
 *
 *  CSU Chico, EECE 555, Fall 2014
 *
 */

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netpacket/packet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pcap/pcap.h>

#include "arptable.h"

#define MAC_ANY "00:00:00:00:00:00"
#define MAC_BCAST "FF:FF:FF:FF:FF:FF"
/* ARP protocol address length (from RFC 826) */
#define ARP_PROLEN 4

#define DEBUG 0

/* {{{ get_srcipmac() */
/*
 * get_srcipmac()
 *
 *   Returns: 0 on success, < 0 on error
 *
 * Get the local ip address and mac for the given interface.
 * 'src_mac' and 'src_ip' will be set by get_srcipmac.
 * They should have enough room allocated as shown below.
 *
 *  char src_mac[INET6_ADDRSTRLEN];	// "00:1c:34:92:11:22"
 *  char src_ip[INET6_ADDRSTRLEN];	// "192.168.1.1"
 *
 *  n = get_srcipmac("eth0", src_ip, src_mac);
 *  if (n < 0)
 *  	exit(1);  // ERROR
 */

int get_srcipmac(char *dev_str, char *src_ip, char *src_mac)
{
	int err;
	struct ifaddrs *ifap, *p;
	struct sockaddr *sa;
	struct sockaddr_in *sin;
	struct sockaddr_ll *sll;
	unsigned char sll_halen;
	int i;

	if ( (err = getifaddrs(&ifap)) != 0) {
		fprintf(stderr, "getifaddrs: %s\n", strerror(errno));
		return -1;
	}

	src_mac[0] = '\0';
	src_ip[0] = '\0';

	/* go through all the interface addresses */
	for (p = ifap; p != NULL; p = p->ifa_next) {
		sa = p->ifa_addr;

		/* skip devices we don't care about */
		if (0 != strcmp(p->ifa_name, dev_str))
			continue;

        if (sa->sa_family == AF_INET) {
			/* get our IP address */
			sin = (struct sockaddr_in *) sa;

			if (NULL == inet_ntop(sin->sin_family, &(sin->sin_addr),
								src_ip, INET6_ADDRSTRLEN)) {
				perror("inet_ntop");
				return -2;
			}
		} else if (sa->sa_family == AF_PACKET) {
			/* get our MAC address */

			sll = (struct sockaddr_ll *) sa;
			sll_halen = sll->sll_halen;

			/* build a string of the MAC address
			 * ab:ef:01:aa:de */
			for (i = 0; i < sll_halen; i++) {
				sprintf(src_mac + i*3, "%02x:", sll->sll_addr[i]);
			}
			/* remove last ':' by ending line one early */
			src_mac[sll_halen*3 - 1] = '\0';
		}
	}

	freeifaddrs(ifap);

	if (src_ip[0] == '\0') {
		fprintf(stderr, "Unable to determine ip\n");
		return -3;
	} else if (src_mac[0] == '\0') {
		fprintf(stderr, "Unable to determine mac\n");
		return -4;
	}

	return 0;
}
/* }}} */

/* {{{ arp_reply() */
/*
 * arp_reply()
 *
 *   Returns: 0 on success, negative on error
 *
 * Inject an arp reply to the address given.
 *
 *   arp_reply(pcap_handle, src_ip, src_mac, dst_ip, dst_mac);
 *   arp_reply(pcap_handle, "192.168.2.1", "00:11:22:33:44:55", ...);
 */
#define SIZEOF_BUF ETHER_HDR_LEN + sizeof(struct ether_arp)
int arp_reply(pcap_t* pcap_handle, char *src_ip, char *src_mac,
									char *dst_ip, char *dst_mac)
{
	u_char packet_data[SIZEOF_BUF];
	struct ether_header *ethhdr = NULL;
	struct ether_arp *ether_arp = NULL;
	struct ether_addr *ethaddr = NULL;

	ethhdr = (struct ether_header *) packet_data;
	ether_arp = (struct ether_arp *) (packet_data + ETHER_HDR_LEN);

	/*
	 * Configure Ethernet header
	 */

	/* destination MAC address */
	ethaddr = ether_aton(dst_mac);
	if (NULL == ethaddr) {
		fprintf(stderr, "Invalid Ethernet destination address.\n");
		return -1;
	}
	memcpy(&ethhdr->ether_dhost, ethaddr, ETH_ALEN);

	/* source MAC address */
	ethaddr = ether_aton(src_mac);  /* our MAC */
	if (NULL == ethaddr) {
		fprintf(stderr, "Invalid Ethernet source address.\n");
		return -2;
	}
	memcpy(&ethhdr->ether_shost, ethaddr, ETH_ALEN);

	/* type */
	ethhdr->ether_type = htons(ETHERTYPE_ARP);

	/*
	 * configure ARP header
	 */

	/* hardware type */
	ether_arp->arp_hrd = htons(ARPHRD_ETHER);

	/* protocol type */
	ether_arp->arp_pro = htons(ETHERTYPE_IP);

	/* hardware address length */
	ether_arp->arp_hln = ETH_ALEN;

	/* protocol address length */
	ether_arp->arp_pln = ARP_PROLEN;

	/* operation */
	ether_arp->arp_op = htons(ARPOP_REPLY);

	/* sender (our) hardware (MAC) address */
	ethaddr = ether_aton(src_mac);
	if (NULL == ethaddr) {
		fprintf(stderr, "Invalid target hardware address.\n");
		return -3;
	}
	memcpy(&ether_arp->arp_sha, ethaddr, ETH_ALEN);

	/* sender (our) protocol (IP) address */
	inet_pton(AF_INET, src_ip, &ether_arp->arp_spa);

	/* target hardware (MAC) address */
	ethaddr = ether_aton(dst_mac);
	if (NULL == ethaddr) {
		fprintf(stderr, "Invalid target hardware address.\n");
		return -4;
	}
	memcpy(&ether_arp->arp_tha, ethaddr, ETH_ALEN);

	/* target protocol (IP) address */
	inet_pton(AF_INET, dst_ip, &ether_arp->arp_tpa);

	/* send the ARP packet */
	pcap_inject(pcap_handle, (void *) packet_data, SIZEOF_BUF);

	return 0;
}
/* }}} */

/* {{{ cleanup() */
/*
 * cleanup()
 *
 * This cleanup() function is used with atexit() to ensure
 * that resources are released no matter where the program exits
 * and without requiring them to be released where the error is
 * detected.
 */

pcap_t *pcap_handle = NULL;  /* Handle for PCAP library */
struct arptable *arptbl = NULL;

int quit = 0;
void int_handler() {
	quit = 1;
}
/* }}} */

/* {{{ is_arp_request() */
/*
 * is_arp_request()
 *
 *   Returns: 1 if an arp request, 0 otherwise
 *
 * Process a received packet and check if it is an ARP request.
 *
 */
int is_arp_request(struct pcap_pkthdr *packet_hdr,
					const u_char *packet_data,
					char *src_ip, char *src_mac,
					char *rqs_ip)
{
	uint32_t len;
	struct ether_header *ethhdr;
	struct ether_arp *ether_arp;
	uint16_t _arp_op;
	uint16_t ether_type;
	char *strp;
	struct in_addr *arp_spa = NULL;
	struct in_addr *arp_tpa = NULL;

	len = packet_hdr->len;

	if (len < sizeof(struct ether_header)) {
		/* too small to have an Ethernet header, ignore */
		return 0;
	}

	ethhdr = (struct ether_header*) packet_data;

	ether_type = ntohs(ethhdr->ether_type);
	if (ether_type == ETHERTYPE_ARP) {
		
		ether_arp = (struct ether_arp*) (packet_data + ETHER_HDR_LEN);

		_arp_op = ntohs(ether_arp->arp_op);

		if (_arp_op != ARPOP_REQUEST)
			return 0;  /* not a request */

		/* sender hardware (MAC) address */
		strp = ether_ntoa((struct ether_addr*) &ether_arp->arp_sha);
		strcpy(src_mac, strp);

		/* sender protocol (IP) address */
		arp_spa = (struct in_addr*) ether_arp->arp_spa;
		strp = inet_ntoa(*arp_spa);
		strcpy(src_ip, strp);

		/* target protocol (IP) address */
		arp_tpa = (struct in_addr*) ether_arp->arp_tpa;
		strp = inet_ntoa(*arp_tpa);
		strcpy(rqs_ip, strp);

		return 1;  /* it was a request */
	}

	return 0;  /* default, not a request */
}
/* }}} */

int main(int argc, char *argv[]) {

	char pcap_buff[PCAP_ERRBUF_SIZE];	/* Error buffer used by pcap */
	char *dev_name = NULL;				/* Device name for live capture */
	char *addr_file = NULL;				/* File with addresses */

	int n;

	int ret;
	struct pcap_pkthdr *packet_hdr = NULL;
	const u_char *packet_data = NULL;

	char src_mac[INET6_ADDRSTRLEN];
	char src_ip[INET6_ADDRSTRLEN];
	char dst_mac[INET6_ADDRSTRLEN];
	char dst_ip[INET6_ADDRSTRLEN];
	char rqs_ip[INET6_ADDRSTRLEN];

	struct sigaction int_act;

	memset(&int_act, 0, sizeof(int_act));
	int_act.sa_handler = int_handler;
	if (-1 == sigaction(SIGINT, &int_act, 0)) {
		perror("int sigaction failed");
		exit(EXIT_FAILURE);
	}

	/* Check command line arguments */
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <net device>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	dev_name  = argv[1];
	addr_file = argv[2];

	/* load the addresses */
	n = load_addrs(&arptbl, addr_file);
	if (n < 0) {
		fprintf(stderr, "load_addrs() failed (%i)\n", n);
		exit(EXIT_FAILURE);
	}

	/* open device */
	pcap_handle = pcap_open_live(dev_name, BUFSIZ, 1, 0, pcap_buff);
	if (pcap_handle == NULL) {
		fprintf(stderr, "Error opening capture device %s: %s\n",
												dev_name, pcap_buff);
		exit(EXIT_FAILURE);
	}

	n = get_srcipmac(dev_name, src_ip, src_mac);
	if (n < 0) {
		fprintf(stderr, "get_srcipmac() failed (%i)\n", n);
		exit(EXIT_FAILURE);
	}

	/* look for ARP requests, send replies */
	while (!quit) {
		/* receive some data */
		ret = pcap_next_ex(pcap_handle, &packet_hdr, &packet_data);
		if (ret < 0)
			continue;  /* timeout or error */

		if (is_arp_request(packet_hdr, packet_data, dst_ip,
													dst_mac, rqs_ip)) {
			if (DEBUG)
				printf("request: %s (%s) for %s\n", dst_ip, dst_mac, rqs_ip);
			if ((mac_lookup(arptbl, rqs_ip, src_mac))) {
				if (DEBUG)
					printf("reply sent\n");
				arp_reply(pcap_handle, rqs_ip, src_mac, dst_ip, dst_mac);
			}
		}
	}

	if (pcap_handle)
		pcap_close(pcap_handle);

	free_arptable(arptbl);

	return EXIT_SUCCESS;
}
