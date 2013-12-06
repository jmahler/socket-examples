
ARGV = -Wall -Wextra

all: arp_resolver

arp_resolver: arp_resolver.c
	gcc $(ARGV) $< -o $@ -lpcap
	sudo setcap CAP_NET_RAW+eip arp_resolver

clean:
	-rm -f arp_resolver

