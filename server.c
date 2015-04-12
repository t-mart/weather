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
#include <fcntl.h>

#include "weather.h"

#define BACKLOG 10     // number of connections we hold in our listen queue

#define LOGFILE "weather.log" // path to log file

int write_log(struct sockaddr_in * sa, char * logline) {

	/*
	 * Write information to the logfile.
	 */
	FILE *f = fopen(LOGFILE, "a+");

	if (f == NULL)
		handle_error("fopen");

	pthread_mutex_lock(&output_file_mutex);
	fprintf(f, "%s", logline);
	INFO_PRINT("%s", logline);
	pthread_mutex_unlock(&output_file_mutex);

	fclose(f);
	return 0;
}

void * process_connection(void * ti) {
	/*
	 * Process a connection: send it a clock response and log the connection
	 * to a log file.
	 *
	 * This function is given to pthread_create.
	 */
	struct thread_info * tinfo = (struct thread_info *) ti;
	char * weather_s = "76.2F\n";
	const char *fmt = "%lld\n";
	int now_s_sz = snprintf(NULL, 0, fmt, NOW) + 1;
	char now_s[now_s_sz];

	char * unit_buf = NULL, * response, logline[1024];
	ssize_t unit_len;

	snprintf(now_s, now_s_sz, fmt, NOW);

	snprintf(logline, 1024, "%lld %s accepted\n", NOW,
		 tinfo->si.ip_port_str);
	write_log(&tinfo->si.sa, logline);

	sendstring(tinfo->si.sock_fd, client_banner);

	do {
		sendstring(tinfo->si.sock_fd, ">>> ");
		snprintf(now_s, now_s_sz, fmt, NOW);
		unit_len = recvunit(&tinfo->si, &unit_buf);
		if (unit_len) {
			if (strncmp(unit_buf, "time", unit_len) == 0)
				response = now_s;
			else if (strncmp(unit_buf, "weather", unit_len) == 0)
				response = weather_s;
			else if (strncmp(unit_buf, "quit", unit_len) == 0)
				break;
			else
				response = help;

			sendstring(tinfo->si.sock_fd, response);
			snprintf(logline, 1024, "%lld %s "
				 "recv'ed=\"%s\" -> send=\"%s\"\n",
				 NOW, tinfo->si.ip_port_str,
				 unit_buf, response);

			write_log(&tinfo->si.sa, logline);
		}
	} while (unit_len);

	close(tinfo->si.sock_fd);
	snprintf(logline, 1024, "%lld %s closed\n", NOW, tinfo->si.ip_port_str);
	write_log(&tinfo->si.sa, logline);

	DESTROY_SOCK_IO(&tinfo->si);
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

	int sfd, accept_sfd;
	int enable=1, rv;
	socklen_t addrlen;
	struct sockaddr_in bind_addr_in, incoming_addr_in;
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


	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		handle_error("socket");
	}
	INFO_PRINT("socket created\n");

	/* prevent problems with lingering socket from previous runs */
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable,
		       sizeof(int)) == -1) {
		handle_error("setsockopt");
	}
	INFO_PRINT("SO_REUSEADDR set\n");

	// AF_INET => IPv4
	// INADDR_ANY => bind to all interfaces
	bind_addr_in = build_addr_in(AF_INET, PORT, INADDR_ANY);

	if (bind(sfd, (struct sockaddr *) &bind_addr_in,
		 sizeof(struct sockaddr_in)) == -1) {
		close(sfd);
		handle_error("bind");
	}
	INFO_PRINT("bound to 0.0.0.0:%d\n", PORT);

	if (listen(sfd, BACKLOG) == -1) {
		handle_error("listen");
	}
	INFO_PRINT("listening...\n");

	while(1) {
		addrlen = sizeof incoming_addr_in;
		accept_sfd = accept(sfd,
				     (struct sockaddr *) &incoming_addr_in,
				     &addrlen);
		if (accept_sfd == -1) {
			handle_error("accept");
		}

		// build thread_info
		tinfo = malloc(sizeof(struct thread_info));
		memset(tinfo, 0, sizeof(struct thread_info)); // clear it
		tinfo->si.sock_fd = accept_sfd;
		tinfo->si.sa = incoming_addr_in;
		INIT_SOCK_IO(&tinfo->si);

		pthread_create(&tinfo->thread_id, &attr, &process_connection,
			       tinfo);
	}

	//clean up
	close(sfd);

	rv = pthread_attr_destroy(&attr);
	if (rv != 0)
		handle_error_en(rv, "pthread_attr_destroy");
	rv = pthread_mutex_destroy(&output_file_mutex);
	if (rv != 0)
		handle_error_en(rv, "pthread_mutex_destroy");

	return 0; // should never get here, user will probably ctrl-c
}
