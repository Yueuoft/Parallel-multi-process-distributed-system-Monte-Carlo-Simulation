CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lm

COMMON_OBJS = mc.o normal_rng.o bs.o

all: single_pricer parallel_pricer server client

single_pricer: single_main.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

parallel_pricer: parallel_main.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server: server.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

client: client.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o single_pricer parallel_pricer server client
