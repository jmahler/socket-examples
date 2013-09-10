
all: oneftp_server oneftp_client

oneftp_server: oneftp_server.c
	gcc -Wall -Wextra -O2 $< -o $@

oneftp_client: oneftp_client.c
	gcc -Wall -Wextra -O2 $< -o $@

clean:
	-rm -f oneftp_server oneftp_client
