CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -D_POSIX_C_SOURCE=200809L
TARGET = moviesearch
OBJS = moviesearch.o parser.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

moviesearch.o: moviesearch.c parser.h
	$(CC) $(CFLAGS) -c moviesearch.c

parser.o: parser.c parser.h
	$(CC) $(CFLAGS) -c parser.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
