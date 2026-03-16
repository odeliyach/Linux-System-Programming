CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2
THREADS := -pthread

SRC_ROOT    := System_Programming_Projects
SHELL_SRCS  := $(SRC_ROOT)/shell/myshell.c $(SRC_ROOT)/shell/shell_main.c
QUEUE_SRCS  := $(SRC_ROOT)/queue/queue.c $(SRC_ROOT)/queue/queue_test.c

.PHONY: all clean test

all: myshell queue_test

myshell: $(SHELL_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

queue_test: $(QUEUE_SRCS)
	$(CC) $(CFLAGS) $(THREADS) -o $@ $^

test: queue_test
	./queue_test

clean:
	rm -f myshell queue_test *.o
