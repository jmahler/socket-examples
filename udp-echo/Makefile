
all: echo_client echo_server

echo_client: echo_client.c
	gcc -Wall -Wextra $< -o $@

echo_server: echo_server.c
	gcc -Wall -Wextra $< -o $@

clean:
	-rm -f echo_client echo_server

