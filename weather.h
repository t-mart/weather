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

#define EOT '\n'

#define MINCMDBUFSZ 2 // min payload size
#define MAXCMDBUFSZ 32 // max payload size

#define INFO_PRINT(...) fprintf(stderr, __VA_ARGS__)

#define DIAG_BUF(buf, len) INFO_PRINT("\tmsg=\""); \
                           print_buffer(buf, len); \
                           INFO_PRINT("\", sz=%zu\n", len);

#define INIT_SOCK_IO(si) (si)->recv_buf = NULL; \
	                       (si)->recv_len = 0; \
                         (si)->ip_port_str = ip_port_string((si)->sa);

#define DESTROY_SOCK_IO(si) free((si)->ip_port_str);

// From PTHREAD_CREATE(3) man pages, 2012-08-03
#define handle_error_en(en, msg) \
                 do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
                 do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct sock_io {
  int sock_fd;
  struct sockaddr_in sa;
  char * ip_port_str;
  char * recv_buf;
  size_t recv_len;
};

struct thread_info {
        pthread_t thread_id;
        struct sock_io si;
};

static char * help = "COMMAND       DESCRIPTION\n"
                     "help          this help\n"
                     "time          print server time\n"
                     "weather       print server weather";

static pthread_mutex_t output_file_mutex = PTHREAD_MUTEX_INITIALIZER;

struct sockaddr_in build_addr_in(sa_family_t fam, int port, uint32_t addr);

char * ip_port_string(struct sockaddr_in si);

char * port_string(struct sockaddr_in si);

char * ip_string(struct sockaddr_in si);

void sendunit(int sock_fd, char * buf, size_t len);

int sendall(int s, char *buf, size_t *len);

void sendstring(int sock_fd, char * buf);

ssize_t recvunit(struct sock_io * si, char ** unit);

ssize_t extract_unit(struct sock_io * si, char ** unit);

int print_buffer(char * buf, size_t len);

int write_log(struct sockaddr_in * sa, char * logline);

