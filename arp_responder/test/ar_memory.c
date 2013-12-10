/*
 * ar_memory.c
 *
 * Load addresses and free the structure and see
 * if any memory is being leaked.  Valgrind is the easiest
 * way to see if memory is being lost.
 *
 *   valgrind --leak-check=yes ./ar_memory
 *
 */

#include "arptable.h"

int main() {
	int n;
	char *file = "../addresses.txt";
	struct arptable *p = NULL;
	int i;

	i = 0;
	while (i++ < 5000) {
		p = NULL;

		n = load_addrs(&p, file);
		if (n < 0) {
			fprintf(stderr, "load_addrs() failed\n");
			exit(1);
		}

		free_arptable(p);
		if (n < 0) {
			fprintf(stderr, "free_arptable() failed\n");
			exit(1);
		}
	}

	return 0;
}
