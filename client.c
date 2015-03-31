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

#define MAXDATASIZE 100 // max number of bytes we can get at once

int main(int argc, char *argv[])
{
	int tcp_socket, numbytes;
	struct sockaddr_in conn_addr_in;
	char s[INET_ADDRSTRLEN];
	char buf[MAXDATASIZE];

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

	if ((numbytes = recv(tcp_socket, buf, MAXDATASIZE-1, 0)) == -1) {
		handle_error("recv");
	}

	buf[numbytes] = '\0';

	printf("%s\n",buf);

	close(tcp_socket);

	return 0;
}
