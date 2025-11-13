#!/bin/bash
#
# run_tests.sh - Automated test runner for BPM IOC
#
# This script builds and runs all tests, generating a report
#

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TESTBENCH_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
LOG_DIR="$TESTBENCH_DIR/logs"
REPORT_FILE="$LOG_DIR/test_report_$(date +%Y%m%d_%H%M%S).txt"

# Create log directory
mkdir -p "$LOG_DIR"

echo -e "${BLUE}======================================"
echo -e "BPM IOC Automated Test Runner"
echo -e "======================================${NC}"
echo ""
echo "Test bench directory: $TESTBENCH_DIR"
echo "Report will be saved to: $REPORT_FILE"
echo ""

# Function to print section header
print_section() {
    echo ""
    echo -e "${YELLOW}======================================"
    echo -e "$1"
    echo -e "======================================${NC}"
    echo ""
}

# Function to run command and log output
run_and_log() {
    local cmd="$1"
    local description="$2"

    echo -e "${BLUE}Running: $description${NC}"
    echo "Command: $cmd"
    echo ""

    # Run command and capture output
    if eval "$cmd" 2>&1 | tee -a "$REPORT_FILE"; then
        echo -e "${GREEN}✓ $description completed successfully${NC}"
        return 0
    else
        echo -e "${RED}✗ $description failed${NC}"
        return 1
    fi
}

# Change to testbench directory
cd "$TESTBENCH_DIR"

# Start test report
{
    echo "======================================"
    echo "BPM IOC Test Report"
    echo "======================================"
    echo "Date: $(date)"
    echo "System: $(uname -a)"
    echo "GCC Version: $(gcc --version | head -n1)"
    echo "======================================"
    echo ""
} > "$REPORT_FILE"

# Track overall status
OVERALL_STATUS=0

# Clean previous builds
print_section "Cleaning Previous Builds"
if run_and_log "make clean" "Clean build"; then
    echo "Clean successful"
else
    echo -e "${YELLOW}Warning: Clean failed, continuing anyway${NC}"
fi

# Build mock driver
print_section "Building Mock Driver Library"
if ! run_and_log "make mock" "Mock driver build"; then
    echo -e "${RED}FATAL: Mock driver build failed${NC}"
    exit 1
fi

# Build unit tests
print_section "Building Unit Tests"
if ! run_and_log "make unit-tests" "Unit tests build"; then
    echo -e "${RED}FATAL: Unit tests build failed${NC}"
    exit 1
fi

# Build test data generator
print_section "Building Test Data Generator"
if ! run_and_log "make test-data" "Test data generator build"; then
    echo -e "${RED}FATAL: Test data generator build failed${NC}"
    exit 1
fi

# Run unit tests
print_section "Running Unit Tests"
if ! run_and_log "make run-unit-tests" "Unit tests execution"; then
    echo -e "${RED}ERROR: Unit tests failed${NC}"
    OVERALL_STATUS=1
else
    echo -e "${GREEN}All unit tests passed${NC}"
fi

# Generate test data
print_section "Generating Test Data"
if ! run_and_log "make run-test-data" "Test data generation"; then
    echo -e "${RED}ERROR: Test data generation failed${NC}"
    OVERALL_STATUS=1
else
    echo -e "${GREEN}Test data generated successfully${NC}"
fi

# Check if test data files were created
print_section "Verifying Test Data Files"
DATA_DIR="$TESTBENCH_DIR/test_data/output"
if [ -d "$DATA_DIR" ]; then
    echo "Test data files created:"
    ls -lh "$DATA_DIR"
    echo ""
    FILE_COUNT=$(ls -1 "$DATA_DIR" | wc -l)
    echo -e "${GREEN}✓ $FILE_COUNT test data files created${NC}"
else
    echo -e "${RED}✗ Test data directory not found${NC}"
    OVERALL_STATUS=1
fi

# Print summary
print_section "Test Summary"
{
    echo ""
    echo "======================================"
    echo "Test Summary"
    echo "======================================"
    echo "Date: $(date)"
    echo ""
} | tee -a "$REPORT_FILE"

if [ $OVERALL_STATUS -eq 0 ]; then
    echo -e "${GREEN}"
    echo "  ✓ All tests PASSED"
    echo -e "${NC}"
    {
        echo "Status: PASSED"
        echo "All tests completed successfully"
    } | tee -a "$REPORT_FILE"
else
    echo -e "${RED}"
    echo "  ✗ Some tests FAILED"
    echo -e "${NC}"
    {
        echo "Status: FAILED"
        echo "Some tests failed - see log for details"
    } | tee -a "$REPORT_FILE"
fi

echo ""
echo "Full report saved to: $REPORT_FILE"
echo ""

exit $OVERALL_STATUS
