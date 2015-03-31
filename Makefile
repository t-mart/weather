CC=gcc
CLANGSPEC=-x c -std=c99
CFLAGS=-Werror -Wall -pedantic
LIBS=-pthread
DEPS=weather.c

.PHONY: test build clean

build: clean server client

server: server.c
	$(CC) $(CLANGSPEC) -o $@ $^ $(DEPS) $(LIBS) $(CFLAGS)

client: client.c
	$(CC) $(CLANGSPEC) -o $@ $^ $(DEPS) $(LIBS) $(CFLAGS)

clean:
	-rm server
	-rm client
