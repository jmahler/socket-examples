
ARGV = -Wall -Wextra -pedantic

all: arp_responder

arp_responder: arp_responder.c arptable.o
	gcc $(ARGV) $< arptable.o -o $@ -lpcap
	sudo setcap CAP_NET_RAW+eip $@

arptable.o: arptable.c arptable.h
	gcc -c $(ARGV) $< -o $@ -lpcap

clean:
	-rm -f arp_responder
	-rm -f *.o

