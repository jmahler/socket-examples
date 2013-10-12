
ARGV = -Wall -Wextra -g

all: snw-client snw-server

# Be sure to create a link to the 32-bit or 64-bit as appropriate
#   ln -s libpacketErrorSendTo_64.a libpacketErrorSendTo.a
libsendto := packetErrorSendTo

arq.o: arq.c arq.h
	gcc $(ARGV) -c $< -o $@ -L. -l$(libsendto)

snw-client: snw-client.c arq.o arq.h 
	gcc $(ARGV) $< arq.o -o $@ -L. -l$(libsendto)

snw-server: snw-server.c arq.o arq.h 
	gcc $(ARGV) $< arq.o -o $@ -L. -l$(libsendto)

clean:
	-rm -f snw-client snw-server arq.o

