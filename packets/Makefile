
ARGV = -Wall -Wextra -pedantic

all: packets

packets: packets.c
	gcc $(ARGV) $< -o $@ -lpcap
	sudo setcap CAP_NET_RAW+eip $@

clean:
	-rm -f packets
