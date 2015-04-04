/*
 * weather.h by Tim Martin 902396824, 2015-03-31
 * =============================================
 *
 * A common header file for weather server/client code.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 7144 // the common port for our program

#define EOT 0

#define MINCMDBUFSZ 2 // min payload size
#define MAXCMDBUFSZ 32 // max payload size

#define INFO_PRINT(...) fprintf(stderr, __VA_ARGS__)

// From PTHREAD_CREATE(3) man pages, 2012-08-03
#define handle_error_en(en, msg) \
                 do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
                 do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct thread_info {
        pthread_t thread_id;
        int sock_fd;
        struct sockaddr_in sa;
};

struct sockaddr_in build_addr_in(sa_family_t fam, int port, uint32_t addr);

const char *  addr_to_buf(struct sockaddr_in * sa, char * buf);

void sendunit(int sock_fd, char * buf, size_t len);

int recvunit(int sock_fd, char * recv_buf, size_t * len, char ** payload_dest,
             size_t * payload_size);

/* int setup_sock_recv_buffer(char * recv_buf, size_t * recv_len); */
int setup_sock_recv_buffer(char ** recv_buf, size_t * recv_len);

int teardown_sock_recv_buffer(char * recv_buf);

extern char weather_cmd;
extern char time_cmd;
