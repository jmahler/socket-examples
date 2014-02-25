#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<netinet/ip.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<math.h>
#include<errno.h>
#include<stdlib.h>

#define MAX_EXPRESSION_LEN 2000
#define MAX_RESPONSE_LEN 20


int process_calculations(int socket_fd) {

	char expression[MAX_EXPRESSION_LEN];
	ssize_t expression_len = 0;
	char done = 0;

	/* Receive the next expression */
	while (!done) {
		if ( (expression_len = recv(socket_fd, expression, MAX_EXPRESSION_LEN, 0)) < 0 ) {
			perror("recv");
			if (close(socket_fd) < 0) {
				perror("close");
			}
			exit(-1);
		} else if (0 == expression_len) {
			/* Done */
			if (close(socket_fd) < 0) {
				perror("close"); exit(-1);
			}
			done = 1;
		} else {
			float a, b, answer;
			char op;
			int ret = sscanf(expression, "%g%c%g", &a, &op, &b);
			ssize_t exp_len = strlen(expression) + 1, len, sent_len = 0;

			if ( (ret == EOF) || (ret < 3) ) {
				answer = NAN;
			} else {
				switch(op) {
					case '*': answer = a * b; break;
					case '/': answer = a / b; break;
					case '+': answer = a + b; break;
					case '-': answer = a - b; break;
					case '^': answer = pow(a,b); break;
					default: answer = NAN; break;
				}
			}
			sprintf(expression, "%g", answer);
			if (exp_len > MAX_RESPONSE_LEN) {
				len = exp_len;
			} else {
				len = MAX_RESPONSE_LEN;
				expression[MAX_RESPONSE_LEN-1] = '\0';
			}
			while (len > 0) {
				if ( (sent_len = send(socket_fd, expression, len, 0)) < 0) {
					perror("send");
					if (close(socket_fd) < 0) {
						perror("close");
					}
					exit(-1);
				}
				len -= sent_len;
			}
		}
	}

	return 0;
}


int main(int argc, char *argv[]) {

	struct addrinfo hints;
	struct addrinfo *results;
	int res = 0;
	int socket_fd = -1;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <listen port>\n", argv[0]);
		return -1;
	}

	/* Get the address structures to use */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ( (res = getaddrinfo(NULL, argv[1], &hints, &results)) != 0 ) {
		fprintf(stderr, "%s\n", gai_strerror(res));
		return -1;
	}

	/* Check we only have one address */
	if (results->ai_next != NULL) {
		fprintf(stderr, "ERROR: Multiple addresses to choose from");
		return -1;
	}

	/* Create the socket */
	if ( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	/* Bind the socket */
	if (bind(socket_fd, results->ai_addr, results->ai_addrlen) < 0) {
		perror("bind");
		return -1;
	}
	freeaddrinfo(results);
	results = 0;

	/* Listen to the socket */
	if (listen(socket_fd, 10) < 0) {
		perror("listen");
		return -1;
	}

	/* Accept and dispatch connections forever */
	while(1) {
		int new_conn;
		int err;

		/* Accept a new connection */
		new_conn = accept(socket_fd, NULL, NULL);
		if (new_conn < 0) {
			perror("accept");
			if (close(socket_fd) < 0) { perror("close"); }
			return -1;
		}

		/* Process calculations until client quits */
		err = process_calculations(new_conn);
		if (err) {
			if (close(socket_fd) < 0) { perror("close"); }
			return -1;
		}
		close(new_conn);
	}

	if ( close(socket_fd) < 0) {
		perror("close");
		return -1;
	}

	return 0;
}


