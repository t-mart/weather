.PHONY: test

test: server
	./server

server: server.c
	clang -x c -std=c99 server.c -o server -D DEBUG -Werror -Wall -pthread
