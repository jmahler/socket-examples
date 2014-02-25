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
#include<pthread.h>
#include<stdlib.h>

#define MAX_EXPRESSION_LEN 2000
#define MAX_RESPONSE_LEN 20

/* Per-thread function */
static void *handle_connection(void *);

/* Per-thread argument */
struct thread_arg_t {
	pthread_t thread_id;
	int socket_fd;			/* Socket for thread to use */
	char done;				/* Set when thread copies data */
};

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
		struct thread_arg_t thread_arg;
		pthread_attr_t thread_attr;

		/* Accept a new connection */
		new_conn = accept(socket_fd, NULL, NULL);
		if (new_conn < 0) {
			perror("accept");
			if (close(socket_fd) < 0) { perror("close"); }
			return -1;
		}

		/* Spawn a new thread for the connection */
		thread_arg.done = 0;
		thread_arg.socket_fd = new_conn;
		if ( (errno = pthread_attr_init(&thread_attr)) ) {
			perror("pthread_attr_init");
			if (close(socket_fd) < 0) { perror("close"); }
			return -1;
		}

		if ( (errno = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED)) ) {
			perror("pthread_attr_setdetachedstate");
			if (close(socket_fd) < 0) { perror("close"); }
			return -1;
		}

		if ( (errno = pthread_create(&(thread_arg.thread_id), &thread_attr, &handle_connection, &thread_arg)) ) {
			perror("pthread_create");
			if (close(socket_fd) < 0) { perror("close"); }
			return -1;
		}

		if ( (errno = pthread_attr_destroy(&thread_attr)) ) {
			perror("pthread_attr_destroy");
			if (close(socket_fd) < 0) { perror("close"); }
			return -1;
		}

		/* Wait for thread to copy data */
		while ( !thread_arg.done ) {}
	}

	if ( close(socket_fd) < 0) {
		perror("close");
		return -1;
	}

	return 0;
}

static void *handle_connection(void *arg) {

	struct thread_arg_t *args_p = (struct thread_arg_t*)arg;
	pthread_t thread_id = args_p->thread_id;
	int socket_fd = args_p->socket_fd;
	char expression[MAX_EXPRESSION_LEN];
	ssize_t expression_len = 0;
	char done = 0;

	args_p->done = 1;
	args_p = NULL;

	/* Receive the next expression */
	while (!done) {
		if ( (expression_len = recv(socket_fd, expression, MAX_EXPRESSION_LEN, 0)) < 0 ) {
			fprintf(stderr, "%x: ", (int)thread_id);
			perror("recv");
			if (close(socket_fd) < 0) {
				fprintf(stderr, "%x: ", (int)thread_id);
				perror("close");
			}
			exit(-1);
		} else if (0 == expression_len) {
			/* Done */
			if (close(socket_fd) < 0) {
				fprintf(stderr, "%x: ", (int)thread_id);
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
					fprintf(stderr, "%x: ", (int)thread_id);
					perror("send");
					if (close(socket_fd) < 0) {
						fprintf(stderr, "%x: ", (int)thread_id);
						perror("close");
					}
					exit(-1);
				}
				len -= sent_len;
			}
		}
	}

	pthread_exit(NULL);
}
