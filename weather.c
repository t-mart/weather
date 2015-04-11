/*
 * weather.c by Tim Martin 902396824, 2015-03-31
 * =============================================
 *
 * A common dependency object.
 */

#include "weather.h"

struct sockaddr_in build_addr_in(sa_family_t fam, int port, uint32_t addr) {
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_family = fam;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = addr;
	return sa;
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
	int len_with_nl = len+1;
	char to[len_with_nl];
	memcpy(to, buf, len);
	to[len] = EOT;
	sendall(sock_fd, to, (int *) &(len_with_nl));
}

void sendstring(int sock_fd, char * buf) {
	// buf MUST be null terminated (else strlen wont work)
	sendunit(sock_fd, buf, strlen(buf));
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

ssize_t recvunit(struct sock_io * si, char ** unit) {
	ssize_t bytes, unit_len;
	size_t recv_len = 50;
	char recv_buf[recv_len];
	while ((unit_len = extract_unit(si, unit)) == -1) {
		memset(recv_buf, 0, recv_len);
		bytes = recv(si->sock_fd, recv_buf, recv_len, 0);
		if (bytes == -1)
			handle_error("recv");
		else if (bytes == 0)
			return 0; //socket closed
		si->recv_buf = realloc(si->recv_buf, si->recv_len + bytes);
		memcpy(si->recv_buf + si->recv_len, recv_buf, bytes);
		si->recv_len += bytes;
	}
	return unit_len;
}

ssize_t extract_unit(struct sock_io * si, char ** unit) {
	char * eot = (char *) memchr(si->recv_buf, EOT, si->recv_len);
	size_t eot_index, leftover_len;
	ssize_t unit_len;
	if (eot != NULL) {
		eot_index = (size_t) (eot - si->recv_buf);

		unit_len = eot_index + 1;
		*unit = realloc(*unit, unit_len);
		memcpy(*unit, si->recv_buf, unit_len);

		leftover_len = si->recv_len - unit_len;

		memmove(si->recv_buf, eot + 1, leftover_len);
		si->recv_buf = realloc(si->recv_buf, leftover_len);
		si->recv_len = leftover_len;

		return unit_len;
	}
	return -1;
}

int print_buffer(char * buf, size_t len) {
	char to[len+1];
	memcpy(to, buf, len);
	to[len] = '\0';
	return fprintf(stderr, "%s", to);
}

char * ip_string(struct sockaddr_in si) {
	char * ips = malloc(INET_ADDRSTRLEN);
	inet_ntop(si.sin_family, &si.sin_addr.s_addr, ips,
		  INET_ADDRSTRLEN);
	return ips;
}

char * port_string(struct sockaddr_in si) {
	const char *port_fmt = "%d";
	int port = ntohs(si.sin_port);
	int port_string_len = snprintf(NULL, 0, port_fmt, (int) port) + 1;
	char * ports = malloc(port_string_len);
	snprintf(ports, port_string_len, port_fmt, port);
	return ports;
}

char * ip_port_string(struct sockaddr_in si) {
	char * ips = ip_string(si);
	char * ports = port_string(si);
	const char *fmt = "%s:%s";
	int len = snprintf(NULL, 0, fmt, ips, ports) + 1;
	char * s = malloc(len);
	snprintf(s, len, fmt, ips, ports);
	free(ips);
	free(ports);
	return s;
}
