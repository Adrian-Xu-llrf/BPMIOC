# Makefile for Test Data Generator

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = -lm

TARGET = generate_test_data

.PHONY: all clean run

all: $(TARGET)

$(TARGET): generate_test_data.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)
	@echo ""
	@echo "Test data generated in output/ directory"

clean:
	rm -f $(TARGET)
	rm -rf output
