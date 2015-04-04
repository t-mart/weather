/*
 * client.c by Tim Martin 902396824, 2015-03-31
 * ============================================
 *
 * Building this source code will create "client".
 *
 * client creates a IPv4 TCP socket. It then connects to a server running on a
 * local interface on the port specified in weather.h (currently 7144). It then
 * receives data from up to a specified size, prints that data to stdout and
 * exits.
 */

#include "weather.h"

char * help = "help          this help\n"
	      "time          print server time\n"
	      "weather       print server weather";

char * prompt = ">>> ";

char * resp_with_string(char * string) {
	char * ret = malloc(strlen(string));
	strncpy(ret, string, strlen(string));
	return ret;
}

char * send_cmd(int sock_fd, char * buf, size_t len) {
	char * recv_buf = NULL, * payload = NULL;
	size_t recv_len = 0, payload_size = 0;
	int rv;

	sendunit(sock_fd, buf, len);

	setup_sock_recv_buffer(&recv_buf, &recv_len);
	rv = recvunit(sock_fd, recv_buf, &recv_len,
		      &payload, &payload_size);
	if (rv == 0)
		// socket close, nothing to do
		return NULL;
	INFO_PRINT("\tmsg=\"%s\", size=%ld\n", payload, payload_size);
	teardown_sock_recv_buffer(recv_buf);
	if (recv_len)
		INFO_PRINT("\tgot rid of recv_buf with \"%s\" (ord=%d, len=%ld) still in it =(\n", recv_buf, *recv_buf, recv_len);

	return payload;
}

int eval_command(int sock_fd, char * buf, size_t len) {
	size_t len_wo_nl = len - 1;
	char * resp;
	if (strncmp(buf, "time", len_wo_nl) == 0
		 || strncmp(buf, "weather", len_wo_nl) == 0) {
		if ((resp = send_cmd(sock_fd, buf, len_wo_nl)) == NULL) {
			INFO_PRINT("\tproblem recving data\n");
			return 0;
		}
		printf("%s\n", resp);
		free(resp);
	} else if (strncmp(buf, "exit", len_wo_nl) == 0)
		return 0;
	else
		printf("%s\n", help);
	return 1;
}

void weather_repl(int sock_fd) {
	char * lineptr = NULL;
	int resp;
	size_t sz;
	ssize_t read;
	while (1) {
		sz = 0;
		printf("%s", prompt);
		read = getline(&lineptr, &sz, stdin);
		if (read == -1)
			handle_error("getline");
		resp = eval_command(sock_fd, lineptr, read);
		free(lineptr);
		lineptr = NULL;
		if (!resp)
			return;
	}
}

int main(int argc, char *argv[])
{
	int tcp_socket;
	struct sockaddr_in conn_addr_in;
	char s[INET_ADDRSTRLEN];

	INFO_PRINT("Weather Client, by Tim Martin 902396824, 2015-03-31\n");

	if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		handle_error("socket");
	}
	INFO_PRINT("socket created\n");

	// AF_INET => IPv4
	// INADDR_ANY => bind to all interfaces
	conn_addr_in = build_addr_in(AF_INET, PORT, INADDR_ANY);

	// we can head straight into connect because this from the ip(7) man
	// pages:
	//  "When connect(2) is called on an unbound socket, the socket is
	//  automatically bound to a random  free port  or  to  a  usable
	//  shared  port  with  the local  address set to INADDR_ANY."
	//
	//  In other words, no bind needed, but we lose control over port
	//  selection...we don't need this anyway
	if (connect(tcp_socket, (struct sockaddr *) &conn_addr_in,
		    sizeof(struct sockaddr_in)) == -1) {
		close(tcp_socket);
		handle_error("bind");
	}

	addr_to_buf(&conn_addr_in, s);
	INFO_PRINT("connected to %s:%d\n", s, PORT);

	weather_repl(tcp_socket);

	close(tcp_socket);

	return 0;
}
