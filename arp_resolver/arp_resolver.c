/*
 * arp_resolver.c
 *
 * Given an IP address as input and it will resolve the MAC address using ARP.
 *
 *   $ sudo ./arp_resolver wlan0
 *   Enter next IP address: 192.168.2.1
 *   MAC: AB:CD:EF:00:12:34
 *   Enter next IP address: 8.8.8.8
 *   MAC: Lookup failed
 *   Enter next IP address:
 *   $
 *
 * The libpcap [www.tcpdump.org] library is used to read the packets.
 *
 * Author:
 *
 *  Jeremiah Mahler <jmmahler@gmail.com>
 *
 *  CSU Chico, EECE 555, Fall 2013
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

#define MAC_ANY "00:00:00:00:00:00"
#define MAC_BCAST "FF:FF:FF:FF:FF:FF"
// ARP protocol address length
#define ARP_PROLEN 4

// Time after which to give up waiting for a response.
#define RESP_TIMEOUT 1

// {{{ setsrcipmac()
/*
 * setsrcipmac()
 *
 *   Returns: 0 on success, < 0 on error
 *
 * Sets the global variables for our source ip (src_ip)
 * and mac address (src_mac).
 */
char src_mac[INET6_ADDRSTRLEN];	// "00:1c:34:92:11:22"
char src_ip[INET6_ADDRSTRLEN];	// "192.168.1.1"

int setsrcipmac(char *dev_str) {
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

	// go through all the interface addresses
	for (p = ifap; p != NULL; p = p->ifa_next) {
		sa = p->ifa_addr;

		// skip devices we don't care about
		if (0 != strcmp(p->ifa_name, dev_str))
			continue;

        if (sa->sa_family == AF_INET) {
			// get our IP address
			sin = (struct sockaddr_in *) sa;

			inet_ntop(sin->sin_family, &(sin->sin_addr), src_ip,sizeof(src_ip));
		} else if (sa->sa_family == AF_PACKET) {
			// get our MAC address

			sll = (struct sockaddr_ll *) sa;
			sll_halen = sll->sll_halen;

			// build a string of the MAC address
			// ab:ef:01:aa:de
			for (i = 0; i < sll_halen; i++) {
				sprintf(src_mac + i*3, "%02x:", sll->sll_addr[i]);
			}
			// remove last ':' by ending line one early
			src_mac[sll_halen*3 - 1] = '\0';
		}
	}

	freeifaddrs(ifap);

	if (src_ip[0] == '\0') {
		fprintf(stderr, "Unable to determine ip\n");
		return -2;
	} else if (src_mac[0] == '\0') {
		fprintf(stderr, "Unable to determine mac\n");
		return -3;
	}

	return 0;
}
// }}}

// {{{ arp_request()
/*
 * arp_request()
 *
 *   Returns: 0 on success, < 0 on error
 *
 * Inject an arp request to the address given.
 *
 * setsrcipmac() must be called once before using this function
 * to set the ip and mac.
 *
 *   setsrcipmac();
 *   arp_inject(pcap_handle, "192.168.2.1");
 */
int arp_request(pcap_t* pcap_handle, char* ipaddr_str) {

	const size_t sizeof_buf = ETHER_HDR_LEN + sizeof(struct ether_arp);
	u_char packet_data[sizeof_buf];
	struct ether_header *ethhdr = NULL;
	struct ether_arp *ether_arp = NULL;
	struct ether_addr *ethaddr = NULL;

	ethhdr = (struct ether_header *) packet_data;
	ether_arp = (struct ether_arp *) (packet_data + ETHER_HDR_LEN);

	//
	// Configure Ethernet header
	//

	// destination MAC address
	ethaddr = ether_aton(MAC_BCAST);  // broadcast
	if (NULL == ethaddr) {
		fprintf(stderr, "Invalid Ethernet destination address.\n");
		return -1;
	}
	memcpy(&ethhdr->ether_dhost, ethaddr, ETH_ALEN);

	// source MAC address
	ethaddr = ether_aton(src_mac);  // our MAC
	if (NULL == ethaddr) {
		fprintf(stderr, "Invalid Ethernet source address.\n");
		return -2;
	}
	memcpy(&ethhdr->ether_shost, ethaddr, ETH_ALEN);

	// type
	ethhdr->ether_type = htons(ETHERTYPE_ARP);

	//
	// configure ARP header
	//

	// hardware type
	ether_arp->arp_hrd = htons(ARPHRD_ETHER);

	// protocol type
	ether_arp->arp_pro = htons(ETHERTYPE_IP);

	// hardware address length
	ether_arp->arp_hln = ETH_ALEN;

	// protocol address length
	ether_arp->arp_pln = ARP_PROLEN;

	// operation
	ether_arp->arp_op = htons(ARPOP_REQUEST);

	// sender (our) hardware (MAC) address
	ethaddr = ether_aton(src_mac);
	if (NULL == ethaddr) {
		fprintf(stderr, "Invalid target hardware address.\n");
		return -3;
	}
	memcpy(&ether_arp->arp_sha, ethaddr, ETH_ALEN);

	// sender (our) protocol (IP) address
	inet_pton(AF_INET, src_ip, &ether_arp->arp_spa);

	// target hardware (MAC) address
	ethaddr = ether_aton(MAC_ANY);
	if (NULL == ethaddr) {
		fprintf(stderr, "Invalid target hardware address.\n");
		return -4;
	}
	memcpy(&ether_arp->arp_tha, ethaddr, ETH_ALEN);

	// target protocol (IP) address
	inet_pton(AF_INET, ipaddr_str, &ether_arp->arp_tpa);

	// send the ARP packet
	pcap_inject(pcap_handle, (void *) packet_data, sizeof_buf);

	return 0;
}
// }}}

pcap_t *pcap_handle = NULL;  // Handle for PCAP library
char *userin = NULL;

// {{{ cleanup()
/*
 * cleanup()
 *
 * This cleanup() function is used with atexit() to ensure
 * that resources are released no matter where the program exits
 * and without requiring them to be released where the error is
 * detected.
 */

void cleanup(void) {
	if (userin)
		free(userin);

	if (pcap_handle)
		pcap_close(pcap_handle);
}
// }}}

// {{{ check_response()
/*
 * check_response()
 *
 *   Returns: 1 if packet was our response, 0 otherwise
 *
 * Process a received packet and check if it is for us.
 * If it is display the result.
 *
 * setsrcipmac() must be called once before using this function
 * to set the ip and mac.
 *
 */
int check_response(struct pcap_pkthdr *packet_hdr, const u_char *packet_data) {

	uint32_t len;
	struct ether_header *ethhdr;
	char *strp;
	struct ether_arp *ether_arp;
	uint16_t _arp_op;
	uint16_t ether_type;

	len = packet_hdr->len;

	if (len < sizeof(struct ether_header)) {
		// too small to have an Ethernet header, not our response
		return 0;
	}

	ethhdr = (struct ether_header*) packet_data;

	// destination mac address
	strp = ether_ntoa((struct ether_addr*) &ethhdr->ether_dhost);
	if (0 != strcmp(strp, src_mac))
		return 0;  // not addressed to our device

	ether_type = ntohs(ethhdr->ether_type);
	if (ether_type == ETHERTYPE_ARP) {
		
		ether_arp = (struct ether_arp*) (packet_data + ETHER_HDR_LEN);

		_arp_op = ntohs(ether_arp->arp_op);

		if (_arp_op != ARPOP_REPLY)
			return 0;  // only interested in replies, not ours

		strp = ether_ntoa((struct ether_addr*) &ether_arp->arp_sha);
		printf("MAC: %s\n", strp);

		return 1;  // got our response
	}

	return 0;  // default, not our response
}
// }}}

void resp_timeout_handler() {
	// tell pcap_next_ex to break
	pcap_breakloop(pcap_handle);
}

int main(int argc, char *argv[]) {

	char pcap_buff[PCAP_ERRBUF_SIZE];       // Error buffer used by pcap
	char *dev_name = NULL;                  // Device name for live capture

	size_t userin_len = 0;

	ssize_t n;

	int ret;
	struct pcap_pkthdr *packet_hdr = NULL;
	const u_char *packet_data = NULL;

	int got_response;

	// set atexit() cleanup function
	if (atexit(cleanup) != 0) {
		fprintf(stderr, "unable to set atexit() function\n");
		exit(EXIT_FAILURE);
	}
	signal(SIGINT, exit);  // catch Ctrl-C/Ctrl-D and exit

	// Check command line arguments
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <net device>\n", argv[0]);
		return -1;
	} else {
		dev_name = argv[1];
	}

	// open device
	pcap_handle = pcap_open_live(dev_name, BUFSIZ, 1, 0, pcap_buff);
	if (pcap_handle == NULL) {
		fprintf(stderr, "Error opening capture device %s: %s\n",
												dev_name, pcap_buff);
		return -1;
	}
	printf("Capturing on interface '%s'\n", dev_name);

	n = setsrcipmac(dev_name);
	if (n < 0) {
		fprintf(stderr, "setsrcipmac() failed\n");
		exit(EXIT_FAILURE);
	}

	while (1) {

		// read user input
		printf("Enter next IP address: ");
		// getline malloc's userin, so free it each time
		if (userin)
			free(userin);
		userin_len = 0;
		n = getline(&userin, &userin_len, stdin);
		if (n < 0) {
			perror("getline failed");
			exit(EXIT_FAILURE);
		}
		// remove new line
		userin[n-1] = '\0';

		// quit if empty line entered
		if (0 == strcmp("", userin))
			break;

		// send an ARP request
		n = arp_request(pcap_handle, userin);
		if (n < 0) {
			printf("ARP request failed\n");
			continue;
		}

		// wait for a response
		got_response = 0;

		alarm(RESP_TIMEOUT);
		signal(SIGALRM, resp_timeout_handler);

		while (1) {
			// receive some data
			ret = pcap_next_ex(pcap_handle, &packet_hdr, &packet_data);
			if (ret < 0)
				break;  // timeout or error

			// if the response is ours, display the result and break.
			if (check_response(packet_hdr, packet_data)) {
				got_response = 1;
				break;
			}
		}
		alarm(0);  // disable timeout
		if (!got_response) {
			printf("MAC: Lookup failed, gave up.\n");		
		}
	}

	return 0;
}
