CROSS_COMPILE = 
CC = $(CROSS_COMPILE)gcc

CFLAGS = -g -Wall -Werror

default: writer

all: writer

writer: writer.o
	$(CC) $< -o $@
	
writer.o: writer.c
	$(CC) $(CFLAGS) -c $< -o $@
	
clean: 
	rm -f writer writer.o

.PHONY: clean
