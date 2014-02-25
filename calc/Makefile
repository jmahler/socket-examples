
ARGV = -Wall -Wextra -pedantic

all: calc_server calc_client

calc_server: calc_server.c
	gcc $(ARGV) -lm $< -o $@

calc_client: calc_client.c
	gcc $(ARGV) $< -o $@

clean:
	-rm -f calc_server calc_client
