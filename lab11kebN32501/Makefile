
APP=lab11kebN32501
SRCS=main.c search_bytes.c 
#SRCS=$(shell find . -name '*.c') # All .c files in current dir
OBJS=$(SRCS:.c=.o)

СС=gcc
CFLAGS= -O3 -Wall -Wextra -Werror
LDFLAGS=-lm

.PHONY: all clean

all: $(APP)

clean:
	@rm -rf *.o $(APP)

$(APP): $(OBJS)
	$(CC) -o $@ $^
.c.o:
	$(CC) $(CFLAGS) -c $<
