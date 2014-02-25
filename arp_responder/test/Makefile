
ARGV = -Wall -Wextra -pedantic -I../

all: ar_memory

# arptable.o must be built first in the parent directory
ar_memory: ar_memory.c
	gcc $(ARGV) $< ../arptable.o -o $@ -lpcap

clean:
	-rm -f ar_memory

