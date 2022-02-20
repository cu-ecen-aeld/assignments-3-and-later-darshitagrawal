CROSS_COMPILE = 
CC = $(CROSS_COMPILE)gcc

CFLAGS = -g -Wall -Werror

default: aesdsocket

all: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $< -o $@
	
aesdsocket.o: aesdsocket.c
	$(CC) $(CFLAGS) -c $< -o $@
	
clean: 
	rm -f aesdsocket aesdsocket.o

.PHONY: clean
