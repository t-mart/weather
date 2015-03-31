/*
 ** client.c -- a stream socket client demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT 3490 // the port client will be connecting to

#define MAXDATASIZE 100 // max number of bytes we can get at once

#define INFO_PRINT(fmt, args...)    fprintf(stderr, fmt, ## args)

int main(int argc, char *argv[])
{
	int tcp_socket, numbytes;
	struct sockaddr_in bind_addr_in;
	struct in_addr bind_addr;
	char s[INET_ADDRSTRLEN];
	char buf[MAXDATASIZE];

	INFO_PRINT("Weather Client, by Tim Martin 902396824, 2015-03-31\n");

	if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	INFO_PRINT("socket created\n");

	memset(&bind_addr, 0, sizeof(struct in_addr));
	bind_addr.s_addr = INADDR_ANY;

	memset(&bind_addr_in, 0, sizeof(struct sockaddr_in)); // clear it (why?)
	bind_addr_in.sin_family = AF_INET;
	bind_addr_in.sin_port = htons(PORT);
	bind_addr_in.sin_addr = bind_addr; // bind to all interfaces

	if (connect(tcp_socket, (struct sockaddr *) &bind_addr_in,
		    sizeof(struct sockaddr_in)) == -1) {
		close(tcp_socket);
		perror("bind");
		exit(EXIT_FAILURE);
	}

	inet_ntop(bind_addr_insin_family,
		  &bind_addr_in.sin_addr.s_addr, s, INET_ADDRSTRLEN);
	INFO_PRINT("connected to %s:%d\n", s, PORT);

	if ((numbytes = recv(tcp_socket, buf, MAXDATASIZE-1, 0)) == -1) {
		perror("recv");
		exit(1);
	}

	buf[numbytes] = '\0';

	printf("%s\n",buf);

	close(tcp_socket);

	return 0;
}
