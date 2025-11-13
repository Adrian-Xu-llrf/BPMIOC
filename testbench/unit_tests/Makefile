# Makefile for BPM IOC Unit Tests

CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -I../mock_driver
LDFLAGS = -L../mock_driver -lmocklowlevel -lm -lpthread

# Test executables
TESTS = test_waveform test_mock_driver

.PHONY: all clean run test

all: $(TESTS)

# Build test_waveform
test_waveform: test_waveform.c test_framework.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Build test_mock_driver
test_mock_driver: test_mock_driver.c test_framework.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Run all tests
test: all
	@echo "======================================"
	@echo "Running all unit tests..."
	@echo "======================================"
	@for test in $(TESTS); do \
		echo ""; \
		echo "Running $$test..."; \
		LD_LIBRARY_PATH=../mock_driver ./$$test || exit 1; \
	done
	@echo ""
	@echo "======================================"
	@echo "All tests completed successfully!"
	@echo "======================================"

# Run tests individually
run: all
	@for test in $(TESTS); do \
		echo "Running $$test..."; \
		LD_LIBRARY_PATH=../mock_driver ./$$test; \
		echo ""; \
	done

# Clean build artifacts
clean:
	rm -f $(TESTS) *.o

# Help target
help:
	@echo "Available targets:"
	@echo "  make all    - Build all tests"
	@echo "  make test   - Build and run all tests"
	@echo "  make run    - Run all tests individually"
	@echo "  make clean  - Remove build artifacts"
