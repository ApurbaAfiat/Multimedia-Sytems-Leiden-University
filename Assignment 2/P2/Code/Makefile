CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -D_POSIX_C_SOURCE=200809L
TARGET = RatingDistance
OBJS   = RatingDistance.o parser.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lm

RatingDistance.o: RatingDistance.c parser.h
	$(CC) $(CFLAGS) -c RatingDistance.c

parser.o: parser.c parser.h
	$(CC) $(CFLAGS) -c parser.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
