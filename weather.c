/*
 * weather.c by Tim Martin 902396824, 2015-03-31
 * =============================================
 *
 * A common dependency object.
 */

#include "weather.h"

struct sockaddr_in build_addr_in(sa_family_t fam, int port, uint32_t addr) {
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(struct sockaddr_in)); // clear it
	sa.sin_family = fam;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = addr;
	return sa;
}

const char * addr_to_buf(struct sockaddr_in * sa, char * buf) {
	return inet_ntop(sa->sin_family, &sa->sin_addr.s_addr, buf,
			 INET_ADDRSTRLEN);
}

