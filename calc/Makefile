
all: calc_server calc_client

calc_server: calc_server.c
	gcc -Wall -Wextra -O2 -lm -lpthread $< -o $@

calc_client: calc_client.c
	gcc -Wall -Wextra -O2 $< -o $@

clean:
	-rm -f calc_server calc_client
