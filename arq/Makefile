
ARGV = -Wall -Wextra -pedantic

all: snw-client snw-server

libpacketErrorSendTo.o: libpacketErrorSendTo.c packetErrorSendTo.h
	gcc $(ARGV) -c -o $@ $<

libpacketErrorSendTo.a: libpacketErrorSendTo.o
	ar rcs $@ $<

libsendto := packetErrorSendTo

arq.o: arq.c arq.h libpacketErrorSendTo.a
	gcc $(ARGV) -c $< -o $@ -L. -l$(libsendto)

snw-client: snw-client.c arq.o arq.h libpacketErrorSendTo.a
	gcc $(ARGV) $< arq.o -o $@ -L. -l$(libsendto)

snw-server: snw-server.c arq.o arq.h libpacketErrorSendTo.a
	gcc $(ARGV) $< arq.o -o $@ -L. -l$(libsendto)

clean:
	-rm -f snw-client snw-server arq.o

