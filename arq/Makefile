
all: snw-client snw-server

# Be sure to create a link to the 32-bit or 64-bit as appropriate
#   ln -s libpacketErrorSendTo_64.a libpacketErrorSendTo.a
libsendto := packetErrorSendTo

snw-client: snw-client.c
	gcc -Wall -Wextra $< -o $@ -L. -l$(libsendto)

snw-server: snw-server.c
	gcc -Wall -Wextra $< -o $@ -L. -l$(libsendto)

clean:
	-rm -f snw-client snw-server

