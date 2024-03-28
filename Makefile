# Makefile for compiling main.c, sort.c, and testData.c

CC := gcc
CFLAGS = -O1 -g -Wall

SRCS := main.c sort.c testData.c

TARGET := code

all: $(TARGET)

# Compile and link the source files
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)
