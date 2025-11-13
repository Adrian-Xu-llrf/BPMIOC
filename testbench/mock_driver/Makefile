# Makefile for Mock Low-Level Driver Library

CC = gcc
CFLAGS = -Wall -Wextra -fPIC -O2 -g
LDFLAGS = -shared
TARGET = libmocklowlevel.so
SYMLINK = liblowlevel.so

SRCS = mock_lowlevel.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean install

all: $(TARGET) $(SYMLINK)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SYMLINK): $(TARGET)
	ln -sf $(TARGET) $(SYMLINK)

clean:
	rm -f $(OBJS) $(TARGET) $(SYMLINK)

install: $(TARGET)
	@echo "Installing mock driver to current directory"
	@echo "To use in tests, set LD_LIBRARY_PATH to include this directory"

# Rebuild on header changes
mock_lowlevel.o: mock_lowlevel.c mock_lowlevel.h
