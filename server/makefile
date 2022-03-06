ifeq ($(CC),)
	CC= $(CROSS_COMPILE)gcc
endif

ifeq ($(CFLAGS),)
	CFLAGS= -g -Wall -Werror -pthread
endif

ifeq ($(LDFLAGS),)
	LDFLAGS = -pthread -lrt	
endif

TARGET=aesdsocket

SRCS= aesdsocket.c

default: ${TARGET}

all: ${TARGET}

$(TARGET): $(SRCS) 
	$(CC) $(CFLAGS) $^ -o $@ $(INCLUDES) $(LDFLAGS)

clean:
	rm -f *.o $(TARGET) *.elf *.map
