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

// Per-thread function
static void *handle_connection(void *);

// Per-thread argument
struct thread_arg_t {
	pthread_t thread_id;
	int socket_fd;			// Socket for thread to use
	char debug;				// Set if thread should print debug info
	char done;				// Set when thread copies data
};

int main(int argc, char *argv[]) {
	if( (argc != 2) && !((argc == 3) && !strncmp(argv[2], "-d", 3)) ) {
		fprintf(stderr, "Usage: %s <listen port> [-d]\n", argv[0]);
		return -1;
	}
	char debug = 0;
	if( argc == 3 ){
		debug = 1;
	}

	/* Get the address structures to use */
	struct addrinfo hints;
	struct addrinfo *results, *rp;
	int res = 0;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if( (res = getaddrinfo(NULL, argv[1], &hints, &results)) != 0 ){
		fprintf(stderr, "%s\n", gai_strerror(res));
		return -1;
	}

	/* Print all the addresses in debug */
	if( debug ){
		struct protoent *proto = NULL;
		struct sockaddr_in *ip = NULL;
		for( rp = results; debug && (rp != NULL); rp = rp->ai_next ){
			switch( rp->ai_family ){
				case AF_INET:
					printf("Family: AF_INET\n");
					char buff[INET_ADDRSTRLEN];
					ip = (struct sockaddr_in*)(rp->ai_addr);
					if( !inet_ntop(ip->sin_family, &(ip->sin_addr), buff, INET_ADDRSTRLEN) ){
						perror("inet_ntop");
						return -1;
					}
					printf("\tAddress: %s\n", buff);
					printf("\tPort: %d\n", ntohs(ip->sin_port));
					break;
				default: printf("Family: UNEXPECTED\n"); break;
			}
			proto = getprotobynumber(rp->ai_protocol);
			printf("\tProtocol: ");
			if( proto ){
				printf("%s\n", proto->p_name);
			}
			else{
				printf("ERROR\n");
			}
		}
		endprotoent();
	}

	/* Check we only have one address */
	if( results->ai_next != NULL ){
		fprintf(stderr, "ERROR: Multiple addresses to choose from");
		return -1;
	}

	/* Create the socket */
	int socket_fd = -1;
	if( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
		perror("socket");
		return -1;
	}
	if( debug ){
		printf("Created socket with fd=%d\n", socket_fd);
	}

	/* Bind the socket */
	if( bind(socket_fd, results->ai_addr, results->ai_addrlen) < 0 ){
		perror("bind");
		return -1;
	}
	freeaddrinfo(results);
	results = 0;
	if( debug ) {
		printf("Socket named successfully\n");
	}

	/* Listen to the socket */
	if( listen(socket_fd, 10) < 0 ){
		perror("listen");
		return -1;
	}
	if( debug ){
		printf("Listening on socket...\n");
	}

	/* Accept and dispatch connections forever */
	while(1) {
		/* Accept a new connection */
		struct sockaddr_in peer;
		socklen_t peer_len = sizeof(struct sockaddr_in);
		int new_conn;
		if( debug ){
			new_conn = accept(socket_fd, (struct sockaddr *)(&peer), &peer_len);
		}
		else{
			new_conn = accept(socket_fd, NULL, NULL);
		}
		if( new_conn < 0 ){
			perror("accept");
			if( close(socket_fd) <0 ){perror("close");}
			return -1;
		}
		if( debug && (peer.sin_family != AF_INET) ){
			fprintf(stderr, "Connection made to incompatible family");
			if( close(socket_fd) <0 ){perror("close");}
			return -1;
		}

		if( debug ){
			char buff[INET_ADDRSTRLEN];
			if( !inet_ntop(peer.sin_family, &(peer.sin_addr), buff, INET_ADDRSTRLEN) ){
				perror("inet_ntop");
				return -1;
			}
			printf("Connected to %s[%d] on fd=%d\n", buff, ntohs(peer.sin_port), new_conn);
		}

		/* Spawn a new thread for the connection */
		struct thread_arg_t thread_arg;
		thread_arg.done = 0;
		thread_arg.debug = debug;
		thread_arg.socket_fd = new_conn;
		pthread_attr_t thread_attr;
		if( (errno = pthread_attr_init(&thread_attr)) ){
			perror("pthread_attr_init");
			if( close(socket_fd) <0 ){perror("close");}
			return -1;
		}
		if( (errno = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED)) ){
			perror("pthread_attr_setdetachedstate");
			if( close(socket_fd) <0 ){perror("close");}
			return -1;
		}
		if( (errno = pthread_create(&(thread_arg.thread_id), &thread_attr, &handle_connection, &thread_arg)) ){
			perror("pthread_create");
			if( close(socket_fd) <0 ){perror("close");}
			return -1;
		}
		if( debug ){
			printf("Spawned thread %x for fd=%d\n", (int)thread_arg.thread_id, new_conn);
		}
		if( (errno = pthread_attr_destroy(&thread_attr)) ){
			perror("pthread_attr_destroy");
			if( close(socket_fd) <0 ){perror("close");}
			return -1;
		}
		/* Wait for thread to copy data */
		while( !thread_arg.done ){}
	}

	if( close(socket_fd) <0 ){
		perror("close");
		return -1;
	}
	return 0;
}

static void *handle_connection(void *arg) {
	struct thread_arg_t *args_p = (struct thread_arg_t*)arg;
	pthread_t thread_id = args_p->thread_id;
	int socket_fd = args_p->socket_fd;
	char debug = args_p->debug;
	args_p->done = 1;
	args_p = NULL;

	if( debug ){
		printf("%x: started with fd=%d\n", (int)thread_id, socket_fd);
	}

	/* Receive the next expression */
	char expression[MAX_EXPRESSION_LEN];
	ssize_t expression_len = 0;
	char done = 0;
	while( !done ){
		if( (expression_len = recv(socket_fd, expression, MAX_EXPRESSION_LEN, 0)) < 0 ){
			fprintf(stderr, "%x: ", (int)thread_id);
			perror("recv");
			if( close(socket_fd) <0 ){
				fprintf(stderr, "%x: ", (int)thread_id);
				perror("close");
			}
			exit(-1);
		}
		else if( expression_len == 0 ){
			/* Done */
			if( close(socket_fd) <0 ){
				fprintf(stderr, "%x: ", (int)thread_id);
				perror("close"); exit(-1);
			}
			done = 1;
			if( debug ){
				printf("%x: Socket closed, ending thread...\n", (int)thread_id);
			}
		}
		else{
			if( debug ){
				printf("%x: Received expression \'%s\'\n", (int)thread_id, expression);
			}
			float a, b, answer;
			char op;
			int ret = sscanf(expression, "%g%c%g", &a, &op, &b);
			if( (ret == EOF) || (ret < 3) ){
				answer = NAN;
			}
			else{
				if( debug ){
					printf("%x: Parsed expression: %f %c %f\n", (int)thread_id, a, op, b);
				}
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
			if( debug ){
				printf("%x: Sending answer \'%s\'\n", (int)thread_id, expression);
			}
			ssize_t exp_len = strlen(expression) + 1, len, sent_len = 0;
			if( exp_len > MAX_RESPONSE_LEN ){
				len = exp_len;
			}
			else{
				len = MAX_RESPONSE_LEN;
				expression[MAX_RESPONSE_LEN-1] = '\0';
			}
			while( len > 0 ){
				if( (sent_len = send(socket_fd, expression, len, 0)) < 0 ){
					fprintf(stderr, "%x: ", (int)thread_id);
					perror("send");
					if( close(socket_fd) <0 ){
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
