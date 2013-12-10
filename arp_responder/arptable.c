/*
 * arptable.c
 *
 * Refer to arptable.h for a description of the functions
 * defined here.
 *
 * Author:
 *
 *  Jeremiah Mahler <jmmahler@gmail.com>
 *
 *  CSU Chico, EECE 555, Fall 2013
 */

#include "arptable.h"

int load_addrs(struct arptable **root, char *file) {

	FILE *fd = NULL;
	char *line = NULL;
	size_t len = 0;
	ssize_t n = 0;
	char ip[INET6_ADDRSTRLEN];
	char mac[INET6_ADDRSTRLEN];
	struct arptable *cur_p;
	struct arptable *new_p;
	int first;
	int ret = 0;

	// It is assumed that atbl (struct arptable)
	// has not defined any entries.

	fd = fopen(file, "r");
	if (NULL == fd) {
		perror("open failed");
		return -1;  // error
	}

	first = 1;
	while (1) {
		n = getline(&line, &len, fd);
		if (n < 0) {
			// eof
			if (0 == errno) {
				ret = 0;  // normal exit
				break;
			}

			// some error
			perror("getline failed");
			ret = -2;
			break;
		}
		len = n;
		if (0 == len) {
			continue;
		}

		// remove newline
		line[len-1] = '\0';

		// get the ip address
		n = sscanf(line, "%s", ip);
		if (n < 0) {
			perror("sscanf ip failed");
			ret = -3;
			break;
		}
		n = strlen(ip);
		if (0 == n) {
			continue;
		}
		// get the mac
		n = sscanf(&line[n+1], "%s", mac);
		if (n < 0) {
			perror("sscanf mac failed");
			ret = -4;
			break;
		}

		// invalid line?
		if (0 == n) {
			printf("skipping line with missing mac\n");
			continue;
		}

		// found an ip and mac
		// add a new entry in to the linked list
		new_p = malloc(sizeof(*new_p));
		strcpy(new_p->ip, ip);
		strcpy(new_p->mac, mac);
		new_p->next = NULL;

		if (first) {
			first = 0;
			(*root) = new_p;
		} else {
			cur_p->next = new_p;
		}
		cur_p = new_p;
	}

	if (line)
		free(line);

	fclose(fd);

	return ret;  // success
}

int mac_lookup(struct arptable *p, char *ip, char *mac)
{
	if (NULL == p)
		return 0;  // reached end, no entry found

	if (0 == strcmp(ip, p->ip)) {
		strcpy(mac, p->mac);
		return 1;  // found an entry
	}

	return mac_lookup(p->next, ip, mac);  // continue searching
}

void free_arptable(struct arptable *p)
{
	struct arptable *pn;

	if (NULL == p)
		return;  // found the end

	pn = p->next;
	free(p);

	free_arptable(pn);
}
