
#include "arptable.h"

int load_addrs(struct arptable *atbl, char *file) {

	FILE *fd = NULL;
	char *line = NULL;
	size_t len = 0;
	ssize_t n = 0;
	char ip[INET6_ADDRSTRLEN];
	char mac[INET6_ADDRSTRLEN];
	struct arptable_entry *cur_p;
	struct arptable_entry *new_p;
	int first;

	// It is assumed that atbl (struct arptable)
	// has not defined any entries.

	fd = fopen(file, "r");
	if (NULL == fd) {
		perror("open failed");
		return -1;  // error
	}

	first = 1;
	while (1) {
		line = NULL;
		n = getline(&line, &len, fd);

		if (n < 0) {
			// eof
			if (0 == errno)
				break;

			// some error
			perror("getline failed");
			return -2;
		}
		len = n;
		if (0 == len)
			continue;

		// remove newline
		line[len-1] = '\0';

		// get the ip address, then the mac
		n = sscanf(line, "%s", ip);
		if (n < 0) {
			perror("sscanf ip failed");
			free(line);
			return -3;
		}
		n = strlen(ip);
		if (0 == n)
			continue;

		n = sscanf(&line[n+1], "%s", mac);
		if (n < 0) {
			perror("sscanf mac failed");
			free(line);
			return -4;
		}
		free(line);
		if (0 == n) {
			printf("skipping line with missing mac\n");
			continue;
		}

		// got an ip and mac
		// add a new entry in to the linked list

		new_p = malloc(sizeof(*new_p));
		strcpy(new_p->ip, ip);
		strcpy(new_p->mac, mac);
		new_p->next = NULL;

		if (first) {
			first = 0;
			atbl->root = new_p;
		} else {
			cur_p->next = new_p;
		}
		cur_p = new_p;
	}

	return 0;  // success
}

int mac_lookup(struct arptable *atbl, char *ip, char *mac) {

	struct arptable_entry *p;

	for (p = atbl->root; p != NULL; p = p->next) {
		if (0 == strcmp(ip, p->ip)) {
			// copy it in to our static buffer
			// and return the pointer.
			strcpy(mac, p->mac);
			return 1;  // found an entry
		}
	}

	return 0;  // no entry found
}

void free_arptable(struct arptable *atbl) {

	struct arptable_entry *p;
	struct arptable_entry *px;

	p = atbl->root;

	// Free all the entries in the linked list.
	while (1) {
		if (NULL == p)
			break;

		px = p;
		p = p->next;
		free(px);
	}
}
