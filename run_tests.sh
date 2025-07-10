#!/bin/bash
# Main test runner script for system monitor validation

echo "=================================="
echo "SYSTEM MONITOR VALIDATION TESTS"
echo "=================================="
echo ""

# Ensure test extractor is built
if [ ! -f "./simple_extractor" ]; then
    echo "Building simple_extractor..."
    g++ -o simple_extractor simple_extractor.cpp
    if [ $? -ne 0 ]; then
        echo "❌ FATAL: Failed to build simple_extractor"
        exit 1
    fi
    echo "✅ simple_extractor built successfully"
    echo ""
fi

# Make all test scripts executable
chmod +x test_*.sh

# Track overall results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=()

# Function to run a test and track results
run_test() {
    local test_name="$1"
    local test_script="$2"
    
    echo "Running $test_name..."
    echo "----------------------------------------"
    
    ((TOTAL_TESTS++))
    
    if ./$test_script; then
        ((PASSED_TESTS++))
        echo "✅ $test_name PASSED"
    else
        FAILED_TESTS+=("$test_name")
        echo "❌ $test_name FAILED"
    fi
    
    echo ""
}

# Run all tests
run_test "User Verification" "test_user.sh"
run_test "Hostname Verification" "test_hostname.sh"
run_test "Process Count Verification" "test_processes.sh"
run_test "Memory Verification" "test_memory.sh"
run_test "Disk Verification" "test_disk.sh"
run_test "Network Verification" "test_network.sh"
run_test "Thermal Verification" "test_thermal.sh"

# Summary
echo "=================================="
echo "TEST SUMMARY"
echo "=================================="
echo "Total tests run: $TOTAL_TESTS"
echo "Tests passed: $PASSED_TESTS"
echo "Tests failed: $((TOTAL_TESTS - PASSED_TESTS))"

if [ ${#FAILED_TESTS[@]} -eq 0 ]; then
    echo ""
    echo "🎉 ALL TESTS PASSED!"
    echo "System monitor implementation matches system commands accurately."
    exit 0
else
    echo ""
    echo "❌ FAILED TESTS:"
    for test in "${FAILED_TESTS[@]}"; do
        echo "  - $test"
    done
    echo ""
    echo "⚠️  Some tests failed. Review the output above for details."
    exit 1
fi