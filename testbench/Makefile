# Top-level Makefile for BPM IOC Testbench

.PHONY: all build test clean help mock unit-tests test-data run-all

# Default target
all: build

# Build all components
build: mock unit-tests test-data
	@echo ""
	@echo "======================================"
	@echo "Testbench build completed!"
	@echo "======================================"

# Build mock driver library
mock:
	@echo "Building mock driver library..."
	@$(MAKE) -C mock_driver
	@echo ""

# Build unit tests
unit-tests: mock
	@echo "Building unit tests..."
	@$(MAKE) -C unit_tests
	@echo ""

# Build test data generator
test-data:
	@echo "Building test data generator..."
	@$(MAKE) -C test_data
	@echo ""

# Run all tests
test: build
	@echo ""
	@echo "======================================"
	@echo "Running BPM IOC Test Suite"
	@echo "======================================"
	@echo ""
	@$(MAKE) -C unit_tests test
	@echo ""
	@echo "======================================"
	@echo "Generating test data..."
	@echo "======================================"
	@$(MAKE) -C test_data run
	@echo ""
	@echo "======================================"
	@echo "All tests completed successfully!"
	@echo "======================================"

# Run tests individually
run-unit-tests: unit-tests
	@$(MAKE) -C unit_tests run

run-test-data: test-data
	@$(MAKE) -C test_data run

# Quick test (just run tests without rebuilding)
quick-test:
	@echo "Running quick tests..."
	@$(MAKE) -C unit_tests run

# Clean all build artifacts
clean:
	@echo "Cleaning testbench..."
	@$(MAKE) -C mock_driver clean
	@$(MAKE) -C unit_tests clean
	@$(MAKE) -C test_data clean
	@echo "Testbench cleaned."

# Deep clean (including test data output)
distclean: clean
	@rm -rf test_data/output
	@echo "Deep clean completed."

# Help message
help:
	@echo "BPM IOC Testbench - Available Targets"
	@echo "======================================"
	@echo ""
	@echo "Build targets:"
	@echo "  make all          - Build all components (default)"
	@echo "  make build        - Same as 'make all'"
	@echo "  make mock         - Build only mock driver library"
	@echo "  make unit-tests   - Build only unit tests"
	@echo "  make test-data    - Build only test data generator"
	@echo ""
	@echo "Test targets:"
	@echo "  make test         - Build and run all tests"
	@echo "  make quick-test   - Run tests without rebuilding"
	@echo "  make run-unit-tests   - Run unit tests only"
	@echo "  make run-test-data    - Generate test data only"
	@echo ""
	@echo "Clean targets:"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make distclean    - Deep clean (including test data)"
	@echo ""
	@echo "Other targets:"
	@echo "  make help         - Show this help message"
	@echo ""

# Install dependencies (documentation only, as this is a self-contained testbench)
install-deps:
	@echo "Testbench dependencies:"
	@echo "  - gcc (GNU C Compiler)"
	@echo "  - make"
	@echo "  - libm (math library)"
	@echo "  - libpthread (thread library)"
	@echo ""
	@echo "These are typically pre-installed on most Linux systems."
