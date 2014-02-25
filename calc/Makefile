
all: calc_server calc_client

calc_server: calc_server.c
	gcc -Wall -Wextra -pedantic -lm $< -o $@

calc_client: calc_client.c
	gcc -Wall -Wextra -pedantic -O2 $< -o $@

clean:
	-rm -f calc_server calc_client
