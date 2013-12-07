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
struct arptable_entry {
	char ip[INET6_ADDRSTRLEN];
	char mac[INET6_ADDRSTRLEN];

	struct arptable_entry *next;
};
/*
 * The ARP table has some config options (file)
 * and the root of the arptable entries.
 */
#define MAXFILESTR 128
struct arptable {
	/* static buffer for address results */
	char strbuf[INET6_ADDRSTRLEN];
	/* file name where addresses are stored */
	char file[MAXFILESTR];

	struct arptable_entry *root;
};

/*
 * load_addrs()
 *
 * Load address from a file.
 * The file should be in the form of ip address and mac pairs.
 *
 *   192.168.99.44	C0:04:AB:43:22:FF
 *   192.168.99.18	D4:DE:AD:BE:EF:FF
 *
 *   load_addrs(arptbl, "addresses.txt");
 *
 * Returns: 0 on success, negative on error
 *
 * It is assumed that this is only done once.
 *
 */
int load_addrs(struct arptable *arptabl, char *file);

/*
 * ip_lookup()
 *
 * Lookup the mac address for a given ip address.
 *
 *   res = ip_lookup(arptbl, "192.168.0.1", mac);
 *
 * Returns true if entry found, false otherwise
 * The 'mac' variable will be set with the address.
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
