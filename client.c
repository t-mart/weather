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

char * prompt = ">>> ";

char * resp_with_string(char * string) {
	char * ret = malloc(strlen(string));
	strncpy(ret, string, strlen(string));
	return ret;
}

ssize_t send_cmd(struct sock_io * si, char * cmd_buf, size_t cmd_len,
		char ** unit_buf) {
	ssize_t unit_len;

	sendunit(si->sock_fd, cmd_buf, cmd_len);

	unit_len = recvunit(si, unit_buf);
	if (unit_len == 0)
		return -1; // socket close, nothing to do

	return unit_len;
}

int eval_command(struct sock_io * si, char * buf, size_t len) {
	size_t len_wo_nl = len - 1;
	char * unit_buf = NULL;
	ssize_t unit_len;

	if (strncmp(buf, "exit", len_wo_nl) == 0)
		return 0;
	else {
		if ((unit_len = send_cmd(si, buf, len_wo_nl, &unit_buf)) == -1) {
			INFO_PRINT("\tproblem recving data\n");
			return 0;
		}
		print_buffer(unit_buf, unit_len);
		INFO_PRINT("\n");
		free(unit_buf);
		return 1;
	}
}

void weather_repl(struct sock_io * si) {
	/* char * lineptr = NULL; */
	/* int resp; */
	/* size_t sz; */
	/* ssize_t read; */
	size_t max_cmd_len = 1024;
	char cmd_buf[max_cmd_len];
	ssize_t bytes;
	while (1) {
		/* sz = 0; */
		/* printf("%s", prompt); */
		/* read = getline(&lineptr, &sz, stdin); */
		/* if (read == -1) */
		/* 	handle_error("getline"); */
		/* resp = eval_command(si, lineptr, read); */
		/* free(lineptr); */
		/* lineptr = NULL; */
		/* if (!resp) */
		/* 	return; */

		/* if (si->recv_len) { */
		/* 	INFO_PRINT("this was also left in the buffer?: \n"); */
		/* 	DIAG_BUF(si->recv_buf, si->recv_len); */
		/* } */
		printf("%s", prompt);
		fflush(stdout);

		bytes = read(0, cmd_buf, max_cmd_len);
		printf("%d\n", (int) bytes);
		if (bytes == -1)
			handle_error("read");
		else if (bytes == 0)
			break;
		sendall(si->sock_fd, cmd_buf, (size_t *) &bytes);
	}
}

int main(int argc, char *argv[])
{
	struct sock_io si;

	INFO_PRINT("Weather Client, by Tim Martin 902396824, 2015-03-31\n");

	if ((si.sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		handle_error("socket");
	}
	INFO_PRINT("socket created\n");

	// AF_INET => IPv4
	// INADDR_ANY => bind to all interfaces
	si.sa = build_addr_in(AF_INET, PORT, INADDR_ANY);

	// we can head straight into connect because this from the ip(7) man
	// pages:
	//  "When connect(2) is called on an unbound socket, the socket is
	//  automatically bound to a random  free port  or  to  a  usable
	//  shared  port  with  the local  address set to INADDR_ANY."
	//
	//  In other words, no bind needed, but we lose control over port
	//  selection...we don't need this anyway
	if (connect(si.sock_fd, (struct sockaddr *) &si.sa,
		    sizeof(struct sockaddr_in)) == -1) {
		close(si.sock_fd);
		handle_error("bind");
	}

	INIT_SOCK_IO(&si);

	INFO_PRINT("connected to %s\n", si.ip_port_str);

	weather_repl(&si);

	close(si.sock_fd);

	DESTROY_SOCK_IO(&si);

	return 0;
}
