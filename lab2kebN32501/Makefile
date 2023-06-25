CC = gcc
CFLAGS = -Wall -Wextra -Werror -O3

all: server client

server: lab2kebN32501_server.c
	$(CC) $(CFLAGS) -o lab2kebN32501_server lab2kebN32501_server.c

client: lab2kebN32501_client.c
	$(CC) $(CFLAGS) -o lab2kebN32501_client lab2kebN32501_client.c

clean:
	rm -f lab2kebN32501_client lab2kebN32501_server
