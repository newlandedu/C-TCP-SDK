all: main

CC=gcc
CFLAGS=-I. -g -lm -lpthread
OBJS = cJSON.o tcpsock.o cloud.o main.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJS)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -f main $(OBJS)
