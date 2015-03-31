/*
 * server.c by Tim Martin 902396824, 2015-03-31
 * ============================================
 *
 * Building this source code will create "server".
 *
 * server creates a IPv4 TCP socket and binds it to all local interfaces on the
 * port specified in weather.h (currently 7144). It then listens for incoming
 * connections and accepts them. After being accepted, an incoming connection
 * will be sent the current clock of the server host and then the connection
 * will be terminated.
 *
 * server logs all incoming connections to the file at the LOGFILE define.
 *
 * After a thread has been accepted, the response and logging are handled by a
 * detached pthread thread.
 */

#include <time.h>

#include "weather.h"

#define BACKLOG 10     // number of connections we hold in our listen queue

#define LOGFILE "weather.log" // path to log file

pthread_mutex_t output_file_mutex = PTHREAD_MUTEX_INITIALIZER;

int write_log(struct sockaddr_in * sa, char * time) {
	/*
	 * Write information to the logfile. Exclusive access is handled by the
	 * caller.
	 */
	FILE *f = fopen(LOGFILE, "a+");
	char s[INET_ADDRSTRLEN];

	addr_to_buf(sa, s);

	if (f == NULL)
		handle_error("fopen");

	fprintf(f, "connection from %s:%d at time %s\n", s,
		ntohs(sa->sin_port), time);

	fclose(f);
	return 0;
}

static void * process_connection(void * tinfo) {
	/*
	 * Process a connection: send it a clock response and log the connection
	 * to a log file.
	 *
	 * This function is given to pthread_create.
	 */
	int * incoming_fd = &((struct thread_info *)tinfo)->incoming_fd;
	struct sockaddr_in * sa = &((struct thread_info *)tinfo)->sa;
	time_t now = time(NULL);
	const char *fmt = "%lld";
	int sz = snprintf(NULL, 0, fmt, (long long) now);
	char now_s[sz + 1]; // note +1 for terminating null byte

	snprintf(now_s, sizeof now_s, fmt, (long long) now);


	if (send(*incoming_fd, now_s, sz, 0) == -1)
		handle_error("send");
	close(*incoming_fd);

	pthread_mutex_lock(&output_file_mutex);
	write_log(sa, now_s);
	pthread_mutex_unlock(&output_file_mutex);

	free(tinfo); // we malloced for this before thread creation

	return NULL;
}

int main(void) {
/*
	From the man pages, and the way things are done in this program,
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
	int enable=1, rv;
	socklen_t addrlen;
	struct sockaddr_in bind_addr_in, incoming_addr_in;
	char s[INET_ADDRSTRLEN];
	pthread_attr_t attr;
	struct thread_info * tinfo;

	INFO_PRINT("Weather Server, by Tim Martin 902396824, 2015-03-31\n"
		   "logging to %s\n"
		   "press ctrl-c to quit\n\n", LOGFILE);

	// init pthread_attr_t
	rv = pthread_attr_init(&attr);
	if (rv != 0)
		handle_error_en(rv, "pthread_attr_init");
	rv = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (rv != 0)
		handle_error_en(rv, "pthread_attr_setdetachstate");


	if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		handle_error("socket");
	}
	INFO_PRINT("socket created\n");

	/* prevent problems with lingering socket from previous runs */
	if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &enable,
		       sizeof(int)) == -1) {
		handle_error("setsockopt");
	}
	INFO_PRINT("SO_REUSEADDR set\n");

	// AF_INET => IPv4
	// INADDR_ANY => bind to all interfaces
	bind_addr_in = build_addr_in(AF_INET, PORT, INADDR_ANY);

	if (bind(tcp_socket, (struct sockaddr *) &bind_addr_in,
		 sizeof(struct sockaddr_in)) == -1) {
		close(tcp_socket);
		handle_error("bind");
	}
	INFO_PRINT("bound to 0.0.0.0:%d\n", PORT);

	if (listen(tcp_socket, BACKLOG) == -1) {
		handle_error("listen");
	}
	INFO_PRINT("listening...\n");

	while(1) {
		addrlen = sizeof incoming_addr_in;
		incoming_fd = accept(tcp_socket,
				     (struct sockaddr *) &incoming_addr_in,
				     &addrlen);
		if (incoming_fd == -1) {
			handle_error("accept");
		}

		addr_to_buf(&incoming_addr_in, s);
		INFO_PRINT("\taccepted connection from %s:%d\n", s,
			   ntohs(incoming_addr_in.sin_port));

		// build thread_info
		tinfo = malloc(sizeof(struct thread_info));
		memset(tinfo, 0, sizeof(struct thread_info)); // clear it
		memcpy(&tinfo->incoming_fd, &incoming_fd, sizeof(int));
		memcpy(&tinfo->sa, &incoming_addr_in,
		       sizeof(struct sockaddr_in));

		pthread_create(&tinfo->thread_id, &attr, &process_connection,
			       tinfo);
	}

	//clean up
	close(tcp_socket);

	rv = pthread_attr_destroy(&attr);
	if (rv != 0)
		handle_error_en(rv, "pthread_attr_destroy");
	rv = pthread_mutex_destroy(&output_file_mutex);
	if (rv != 0)
		handle_error_en(rv, "pthread_mutex_destroy");

	return 0; // should never get here, user will probably ctrl-c
}
