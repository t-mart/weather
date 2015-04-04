/*
 * weather.c by Tim Martin 902396824, 2015-03-31
 * =============================================
 *
 * A common dependency object.
 */

#include "weather.h"

char weather_cmd = 'w';
char time_cmd = 't';

struct sockaddr_in build_addr_in(sa_family_t fam, int port, uint32_t addr) {
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_family = fam;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = addr;
	return sa;
}

const char * addr_to_buf(struct sockaddr_in * sa, char * buf) {
	return inet_ntop(sa->sin_family, &sa->sin_addr.s_addr, buf,
			 INET_ADDRSTRLEN);
}

int setup_sock_recv_buffer(char ** recv_buf, size_t * recv_len) {
	if ((*recv_buf = malloc(2*MAXCMDBUFSZ)) == NULL)
		handle_error("malloc");
	*recv_len = 0;
	return 1;
}

int teardown_sock_recv_buffer(char * recv_buf) {
	free(recv_buf);
	return 1;
}

int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
	    n = send(s, buf+total, bytesleft, 0);
	    if (n == -1) { break; }
	    total += n;
	    bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}

void sendunit(int sock_fd, char * buf, size_t len) {
	char eot = EOT;
	sendall(sock_fd, buf, (int *) &len);
	len = 1;
	sendall(sock_fd, &eot, (int *) &len);
}

int find_term_index(char * recv_buf, size_t recv_len) {
	int i = 0;
	char c;
	for (c = recv_buf[i]; i < recv_len; c = recv_buf[++i]) {
		if (c == EOT)
			return i;
	}
	return -1;
}

int extract_unit(char * recv_buf, size_t * len, char ** payload_dest,
	     size_t * payload_len) {
	//index of EOT, include this
	int term_index = find_term_index(recv_buf, *len);
	if (term_index >= 0) {
		*payload_len = term_index + 1;
		*payload_dest = malloc(*payload_len);
		if (*payload_dest == NULL)
			handle_error("malloc");
		memcpy(*payload_dest, recv_buf, *payload_len);
		memmove(recv_buf, recv_buf + *payload_len,
			*len - *payload_len);
		*len -= *payload_len;
		return 1;
	}
	return 0;
}

int recvunit(int sock_fd, char * recv_buf, size_t * len, char ** payload_dest,
	     size_t * payload_len) {
	// returns the size of the received buffer placed in dest.
	// dest must be NULL on call
	ssize_t bytes;

	while (1) {
		if (*len >= MINCMDBUFSZ) {
			if (extract_unit(recv_buf, len, payload_dest,
					 payload_len) == 1)
				return 1;

		}
		bytes = recv(sock_fd, recv_buf + *len,
				(2*MAXCMDBUFSZ) - *len, 0);
		if (bytes == -1)
			handle_error("recv");
		else if (bytes == 0)
			return 0;
		*len += bytes;
	}
	return 0;
}

