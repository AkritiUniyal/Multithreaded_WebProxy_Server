CC=g++
CFLAGS= -g -Wall -I/usr/include/jansson
LDFLAGS= -lpthread -ljansson

all: proxy

proxy_parse.o: proxy_parse.c proxy_parse.h
	$(CC) $(CFLAGS) -c proxy_parse.c -o proxy_parse.o

proxy.o: proxy_server_with_cache.c
	$(CC) $(CFLAGS) -c proxy_server_with_cache.c -o proxy.o

proxy: proxy_parse.o proxy.o
	$(CC) $(CFLAGS) -o proxy proxy_parse.o proxy.o $(LDFLAGS)

clean:
	rm -f proxy *.o
