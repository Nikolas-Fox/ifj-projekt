CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -g
 
SRCS    = hashtable.c symtable.c
HEADERS = hashtable.h symtable.h
OBJS    = $(SRCS:.c=.o)
 
.PHONY: all build check build-check clean
 

all: build
 
build: $(OBJS)
 
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@
 
check:
	clang-format --style=file --dry-run -Werror $(SRCS) $(HEADERS)
 
build-check: check build
 
clean:
	rm -f $(OBJS)
 
