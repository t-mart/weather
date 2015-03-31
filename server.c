/*
 ** server.c -- a stream socket server demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#define PORT 3490  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

#define TIMELEN 16     // size of the time in characters

#define INFO_PRINT(fmt, args...)    fprintf(stderr, fmt, ## args)

static void * process_connection(void * fd_storage) {
	int incoming_fd = *(int *)fd_storage;
	time_t now = time(NULL);
	const char *fmt = "%lld";
	int sz = snprintf(NULL, 0, fmt, (long long) now);
	char now_s[sz + 1]; // note +1 for terminating null byte
	snprintf(now_s, sizeof now_s, fmt, (long long) now);

	free(fd_storage); // we malloced for this before thread creation

	if (send(incoming_fd, now_s, TIMELEN, 0) == -1)
		perror("send");
	close(incoming_fd);

	// do output writing here

	return NULL;
}

int main(void)
{
/*
	From the man pages and the way things are done in this program,
	     "To accept connections, the following steps are performed:
		     1.  A socket is created with socket(2).
		     2.  The  socket is bound to a local address using bind(2),
			 so that other sockets may be connect(2)ed to it.
		     3.  A willingness to accept incoming connections and a
			 queue limit for incoming connections are specified
			 with listen().
		     4.  Connections are accepted with accept(2)."
*/

	int tcp_socket, incoming_fd;
	int enable=1;
	socklen_t addrlen;
	struct sockaddr_in bind_addr_in, incoming_addr_in;
	struct in_addr bind_addr;
	char s[INET_ADDRSTRLEN];
	pthread_t tid;
	/* pthread_mutex_t output_file_mutex; */
	int * fd_storage;

	INFO_PRINT("Weather Server, by Tim Martin 902396824, 2015-03-31\n"
		   "press ctrl-c to quit\n\n");

	if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	INFO_PRINT("socket created\n");

	/* prevent problems with lingering socket from previous runs */
	if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &enable,
		       sizeof(int)) == -1) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	INFO_PRINT("SO_REUSEADDR set\n");

	memset(&bind_addr, 0, sizeof(struct in_addr));
	bind_addr.s_addr = INADDR_ANY;

	memset(&bind_addr_in, 0, sizeof(struct sockaddr_in)); // clear it (why?)
	bind_addr_in.sin_family = AF_INET;
	bind_addr_in.sin_port = htons(PORT);
	bind_addr_in.sin_addr = bind_addr; // bind to all interfaces

	if (bind(tcp_socket, (struct sockaddr *) &bind_addr_in,
		 sizeof(struct sockaddr_in)) == -1) {
		close(tcp_socket);
		perror("bind");
		exit(EXIT_FAILURE);
	}
	INFO_PRINT("bound\n");

	if (listen(tcp_socket, BACKLOG) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	INFO_PRINT("listening...\n");

	while(1) {  // main accept() loop
		addrlen = sizeof incoming_addr_in;
		incoming_fd = accept(tcp_socket,
				     (struct sockaddr *) &incoming_addr_in,
				     &addrlen);
		if (incoming_fd == -1) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		inet_ntop(incoming_addr_in.sin_family,
			  &incoming_addr_in.sin_addr.s_addr,
			  s, INET_ADDRSTRLEN);
		INFO_PRINT("\taccepted connection from %s\n", s);

		fd_storage = malloc(sizeof(int));
		memcpy(fd_storage, &incoming_fd, sizeof(int));

		pthread_create(&tid, NULL, &process_connection, fd_storage);

	}

	close(tcp_socket);
	return 0;
}

