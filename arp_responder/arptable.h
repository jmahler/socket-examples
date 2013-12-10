/*
 * arptable.h
 *
 * This ARP table library allows a table of ip address
 * and MAC address pairs to be stored in a file.  This
 * file can then be loaded and used to lookup entries.
 *
 *   char *addr_file = "addresses.txt"
 *   struct arptable *arptbl;
 *   char mac[INET6_ADDRSTRLEN];
 *   int n;
 *
 *   n = load_addrs(&arptbl, addr_file);
 *   n = mac_lookup(arptbl, "192.168.2.1", &mac);
 *
 *   free_arptable(arptbl);
 *
 * Author:
 *
 *  Jeremiah Mahler <jmmahler@gmail.com>
 *
 *  CSU Chico, EECE 555, Fall 2013
 *
 */

#ifndef _ARPTABLE_H
#define _ARPTABLE_H

#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * The table of ARP ip/mac entries is stored
 * as a linked list.
 */
struct arptable {
	char ip[INET6_ADDRSTRLEN];
	char mac[INET6_ADDRSTRLEN];

	struct arptable *next;
};

/*
 * load_addrs()
 *
 * Load address from a file.
 *
 *   load_addrs(arptbl, "addresses.txt");
 *
 * Returns: 0 on success, negative on error
 *
 * The file should be in the form of ip address and mac pairs.
 *
 *   192.168.99.44	C0:04:AB:43:22:FF
 *   192.168.99.18	D4:DE:AD:BE:EF:FF
 *
 * It is assumed that this is only done once.
 * If it must be done multiple times free_arptable() should
 * be called between uses.
 *
 */
int load_addrs(struct arptable **arptabl, char *file);

/*
 * ip_lookup()
 *
 * Lookup the mac address for a given ip address.
 *
 *   char mac[INET6_ADDRSTRLEN];
 *
 *   res = ip_lookup(arptbl, "192.168.0.1", &mac);
 *
 * 'mac' should be a buffer of INET6_ADDRSTRLEN bytes
 * to store the answer.
 *
 * Returns true if entry found, false otherwise
 * The 'mac' variable will be set with the address.
 *
 */
int mac_lookup(struct arptable *arptable, char *ip, char *mac);

/*
 * free_arptable()
 *
 * Clear all the table entries and de-allocate memory.
 *
 */
void free_arptable(struct arptable *arptable);

#endif
