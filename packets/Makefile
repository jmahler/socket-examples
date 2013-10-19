
ARGV = -Wall -Wextra -g

all: packets

packets: packets.c
	gcc $(ARGV) $< -o $@ -lpcap

clean:
	-rm -f packets
