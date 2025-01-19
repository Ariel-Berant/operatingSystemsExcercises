CC = gcc
CFLAGS = -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 -pthread

all: queue_test

queue.o: queue.c
	$(CC) $(CFLAGS) -c queue.c

queue_test: queue.o queue_test.c
	$(CC) $(CFLAGS) queue.o queue_test.c -o queue_test

clean:
	rm -f *.o queue_test
