
ARGV=-Wall -Wextra -pedantic

all: echo_client echo_server

echo_client: echo_client.c
	gcc $(ARGV) $< -o $@

echo_server: echo_server.c
	gcc $(ARGV) $< -o $@

clean:
	-rm -f echo_client echo_server

