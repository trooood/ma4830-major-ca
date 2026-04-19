# Makefile for wavegen
CC = cc
CFLAGS = -Wall -Isrc
LDFLAGS = -lm -lpthread
TARGET = wavegen
# Finds main.c in root and all .c files in the src directory
SRCS = main.c $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)