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


int process_calculations(int sockfd) {

	char expression[MAX_EXPRESSION_LEN];
	ssize_t expression_len = 0;
	char done = 0;

	/* Receive the next expression */
	while (!done) {
		if ( (expression_len = recv(sockfd, expression, MAX_EXPRESSION_LEN, 0)) < 0 ) {
			perror("recv");
			if (close(sockfd) < 0) {
				perror("close");
			}
			exit(-1);
		} else if (0 == expression_len) {
			/* Done */
			if (close(sockfd) < 0) {
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
				if ( (sent_len = send(sockfd, expression, len, 0)) < 0) {
					perror("send");
					if (close(sockfd) < 0) {
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
	int sockfd = -1;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <listen port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Get the address structures to use */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ( (res = getaddrinfo(NULL, argv[1], &hints, &results)) != 0 ) {
		fprintf(stderr, "%s\n", gai_strerror(res));
		exit(EXIT_FAILURE);
	}

	/* Check we only have one address */
	if (results->ai_next != NULL) {
		fprintf(stderr, "ERROR: Multiple addresses to choose from");
		exit(EXIT_FAILURE);
	}

	/* Create the socket */
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/* Bind the socket */
	if (bind(sockfd, results->ai_addr, results->ai_addrlen) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(results);
	results = 0;

	/* Listen to the socket */
	if (listen(sockfd, 10) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	/* Accept and dispatch connections forever */
	while(1) {
		int new_conn;
		int err;

		/* Accept a new connection */
		new_conn = accept(sockfd, NULL, NULL);
		if (new_conn < 0) {
			perror("accept");
			if (close(sockfd) < 0) { perror("close"); }
			return -1;
		}

		/* Process calculations until client quits */
		err = process_calculations(new_conn);
		if (err) {
			if (close(sockfd) < 0) { perror("close"); }
			return -1;
		}
		close(new_conn);
	}

	if (close(sockfd) < 0) {
		perror("close");
		return -1;
	}

	return 0;
}


