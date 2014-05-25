/*
 * packets.c
 *
 * Will monitor live network traffic or process a .pcap dump
 * and display basic header information about the packets.
 *
 *   $ sudo ./packets eth0
 *
 *   48:5b:39:5b:6b:87 -> 1:80:c2:0:0:0 [len:46] 
 *   48:5b:39:5b:6b:87 -> 1:80:c2:0:0:0 [len:46] 
 *   0:1a:70:5a:6e:9 -> 8c:70:5a:83:2b:64 [ARP] 192.168.2.1 requests 192.168.2.113 
 *   8c:70:5a:83:2b:64 -> 0:1a:70:5a:6e:9 [ARP] 192.168.2.113 at 8c:70:5a:83:2b:64 
 *   48:5b:39:5b:6b:87 -> 1:80:c2:0:0:0 [len:46] 
 *   48:5b:39:5b:6b:87 -> 1:80:c2:0:0:0 [len:46] 
 *   8c:70:5a:83:2b:64 -> 0:1a:70:5a:6e:9 [IPv4] 192.168.2.113 -> 8.8.8.8 
 *   0:1a:70:5a:6e:9 -> 8c:70:5a:83:2b:64 [IPv4] 8.8.8.8 -> 192.168.2.113 
 *   8c:70:5a:83:2b:64 -> 0:1a:70:5a:6e:9 [IPv4] 192.168.2.113 -> 149.20.4.69 
 *   0:1a:70:5a:6e:9 -> 8c:70:5a:83:2b:64 [IPv4] 149.20.4.69 -> 192.168.2.113 
 *   8c:70:5a:83:2b:64 -> 0:1a:70:5a:6e:9 [IPv4] 192.168.2.113 -> 8.8.8.8 
 *   0:1a:70:5a:6e:9 -> 8c:70:5a:83:2b:64 [IPv4] 8.8.8.8 -> 192.168.2.113 
 *
 * To process a capture give the file name as an argument.
 *
 *   $ sudo ./packets v6-http.cap
 *
 * The libpcap [www.tcpdump.org] library is used to read the packets.
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
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pcap/pcap.h>

#define MAXSTR 128

int main(int argc, char *argv[]) {

	char pcap_buff[PCAP_ERRBUF_SIZE];       /* Error buffer used by pcap */
	pcap_t *pcap_handle = NULL;             /* Handle for PCAP library */
	struct pcap_pkthdr *packet_hdr = NULL;  /* Packet header from PCAP */
	const u_char *packet_data = NULL;       /* Packet data from PCAP */
	int ret = 0;                            /* Return value from library calls */
	char *file_or_dev = NULL;
	uint32_t len;

	struct ether_header *ethhdr = NULL;
	char *strp = NULL;
	char str[MAXSTR];
	uint16_t ether_type;

	struct ip *ip = NULL;

	struct ether_arp *ether_arp = NULL;
	uint16_t _arp_op;
	struct in_addr *arp_spa = NULL;
	struct in_addr *arp_tpa = NULL;

	struct ip6_hdr *ip6_hdr = NULL;

	uint16_t *vlan_id;

	/* Check command line arguments */
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file | device>\n", argv[0]);
		exit(EXIT_FAILURE);
	} else {
		file_or_dev = argv[1];
	}

	/* Try to open as a file and if that doesn't work
	 * try to open as a device. */
	pcap_handle = pcap_open_offline(file_or_dev, pcap_buff);
	if (NULL == pcap_handle) {
		pcap_handle = pcap_open_live(file_or_dev, BUFSIZ, 1, 0, pcap_buff);
		if (NULL == pcap_handle) {
			fprintf(stderr, "Error opening device (or file?) %s: %s\n",
												file_or_dev, pcap_buff);
			exit(EXIT_FAILURE);
		} else {
			printf("Capturing on interface '%s'\n", file_or_dev);
		}
	} else {
		printf("Processing file '%s'\n", file_or_dev);
	}

	while (1) {

		ret = pcap_next_ex(pcap_handle, &packet_hdr, &packet_data);

		if (ret == -2) {
			/* trace ended */
			break;
		} else if (ret == -1) {
			/* An error occurred */
			pcap_perror(pcap_handle, "Error processing packet:");
			pcap_close(pcap_handle);
			return -1;
		} else if (ret == 0) {
			/* live capture timeout */
			continue;
		} else if (ret != 1) {
			/* Unexpected return values; other values shouldn't happen
			 * when reading trace files */
			fprintf(stderr, "Unexpected return val (%i) from pcap_next_ex()\n",
																ret);
			pcap_close(pcap_handle);
			exit(EXIT_FAILURE);
		} else {
			/* Process the packet and print results */

			len = packet_hdr->len;

			if (packet_hdr->caplen != len) {
				fprintf(stderr, "Partial packet, discarding.\n");
				continue;
			}

			if (len < sizeof(struct ether_header)) {
				fprintf(stderr, "Not enough data for Ethernet, discarding.\n");
				continue;
			}

			ethhdr = (struct ether_header*) packet_data;

			/* source mac address */
			strp = ether_ntoa((struct ether_addr*) &ethhdr->ether_shost);
			printf("%s -> ", strp);

			/* destination mac address */
			strp = ether_ntoa((struct ether_addr*) &ethhdr->ether_dhost);
			printf("%s ", strp);

			/* Ethernet type */
			ether_type = ntohs(ethhdr->ether_type);
			if (ether_type <= ETH_DATA_LEN) {
				/* length */
				printf("[len:%i] ", ether_type);
			} else if (ether_type == ETHERTYPE_IP) {
				printf("[IPv4] ");

				if (len < ETHER_HDR_LEN + sizeof(struct ip)) {
					fprintf(stderr, "IP header is too small, discarding\n");
					continue;
				}

				ip = (struct ip*) (packet_data + ETHER_HDR_LEN);

				/* IP source address */
				inet_ntop(AF_INET, &ip->ip_src, str, MAXSTR);
				printf("%s -> ", str);

				/* IP destination address */
				inet_ntop(AF_INET, &ip->ip_dst, str, MAXSTR);
				printf("%s ", str);
			} else if (ether_type == ETHERTYPE_ARP) {
				printf("[ARP] ");

				if (len < ETHER_HDR_LEN + sizeof(struct ether_arp)) {
					fprintf(stderr, "ARP header is too small, discarding\n");
					continue;
				}

				ether_arp = (struct ether_arp*) (packet_data + ETHER_HDR_LEN);

				_arp_op = ntohs(ether_arp->arp_op);

				if (_arp_op == ARPOP_REQUEST) {
					/* sender protocol address (spa) */
					arp_spa = (struct in_addr*) ether_arp->arp_spa;
					printf("%s ", inet_ntoa(*arp_spa));

					printf("requests ");

					/* target protocol address (tpa) */
					arp_tpa = (struct in_addr*) ether_arp->arp_tpa;
					printf("%s ", inet_ntoa(*arp_tpa));
				} else if (_arp_op == ARPOP_REPLY) {
					/* sender protocol address (spa) */
					arp_spa = (struct in_addr*) ether_arp->arp_spa;
					printf("%s ", inet_ntoa(*arp_spa));

					printf("at ");

					/* sender hardware address (sha) */
					strp = ether_ntoa((struct ether_addr*) &ether_arp->arp_sha);
					printf("%s ", strp);

				} else {
					/* unknown */
					printf("?:0x%.2x ", _arp_op);
				}

			} else if (ether_type == ETHERTYPE_VLAN) {
				printf("[VLAN] ");

				if (len < ETHER_HDR_LEN + sizeof(uint16_t)) {
					fprintf(stderr, "VLAN header is too small, discarding\n");
					continue;
				}

				/* VLAN ID */
				vlan_id = (uint16_t*) (packet_data + ETHER_HDR_LEN);
				*vlan_id = ntohs(*vlan_id) & 0xFFF;
				printf("ID = %i ", *vlan_id);

			} else if (ether_type == ETHERTYPE_IPV6) {
				printf("[IPv6] ");

				if (len < ETHER_HDR_LEN + sizeof(struct ip6_hdr)) {
					fprintf(stderr, "IPv6 header is too small, discarding\n");
					continue;
				}

				ip6_hdr = (struct ip6_hdr*) (packet_data + ETHER_HDR_LEN);

				inet_ntop(AF_INET6, &ip6_hdr->ip6_src, str, MAXSTR);
				printf("%s -> ", str);
				inet_ntop(AF_INET6, &ip6_hdr->ip6_dst, str, MAXSTR);
				printf("%s ", str);
			} else {
				printf("[Other] ");
			}

			printf("\n");
		}
	}

	pcap_close(pcap_handle);
	return 0;
}
