
ARGV = -Wall -Wextra

all: arp_responder

arp_responder: arp_responder.c
	gcc $(ARGV) $< -o $@ -lpcap
	sudo setcap CAP_NET_RAW+eip arp_responder

clean:
	-rm -f arp_responder

