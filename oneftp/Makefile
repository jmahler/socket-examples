
ARGV = -Wall -Wextra -pedantic

all: oneftp_server oneftp_client

oneftp_server: oneftp_server.c
	gcc $(ARGV) $< -o $@

oneftp_client: oneftp_client.c
	gcc $(ARGV) $< -o $@

clean:
	-rm -f oneftp_server oneftp_client
