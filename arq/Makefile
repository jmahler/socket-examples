
ARGV = -Wall -Wextra -pedantic

all: snw-client snw-server

libunreliable_sendto.o: libunreliable_sendto.c unreliable_sendto.h
	gcc $(ARGV) -c -o $@ $<

libunreliable_sendto.a: libunreliable_sendto.o
	ar rcs $@ $<

libsendto := unreliable_sendto

arq.o: arq.c arq.h libunreliable_sendto.a
	gcc $(ARGV) -c $< -o $@ -L. -l$(libsendto)

snw-client: snw-client.c arq.o arq.h libunreliable_sendto.a
	gcc $(ARGV) $< arq.o -o $@ -L. -l$(libsendto)

snw-server: snw-server.c arq.o arq.h libunreliable_sendto.a
	gcc $(ARGV) $< arq.o -o $@ -L. -l$(libsendto)

clean:
	-rm -f snw-client snw-server *.a *.o

