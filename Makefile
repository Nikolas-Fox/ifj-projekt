CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic

TARGET = test_hashtable


SRCS = main.c hashtable.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean


