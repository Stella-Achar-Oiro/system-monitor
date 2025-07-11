#!/bin/bash
# Test script to verify memory information matches free command

echo "=== MEMORY VERIFICATION TEST ==="

# Get our implementation's memory info
OUR_OUTPUT=$(./simple_extractor memory)
OUR_TOTAL=$(echo "$OUR_OUTPUT" | grep "MemTotal:" | awk '{print $2}')
OUR_FREE=$(echo "$OUR_OUTPUT" | grep "MemFree:" | awk '{print $2}')
OUR_AVAILABLE=$(echo "$OUR_OUTPUT" | grep "MemAvailable:" | awk '{print $2}')

# Get system memory info using free -k (kB)
FREE_OUTPUT=$(free -k)
SYSTEM_TOTAL=$(echo "$FREE_OUTPUT" | grep "Mem:" | awk '{print $2}')
SYSTEM_FREE=$(echo "$FREE_OUTPUT" | grep "Mem:" | awk '{print $4}')
SYSTEM_AVAILABLE=$(echo "$FREE_OUTPUT" | grep "Mem:" | awk '{print $7}')

echo "Memory Total:"
echo "  Our implementation: $OUR_TOTAL kB"
echo "  free command: $SYSTEM_TOTAL kB"

echo "Memory Free:"
echo "  Our implementation: $OUR_FREE kB"
echo "  free command: $SYSTEM_FREE kB"

echo "Memory Available:"
echo "  Our implementation: $OUR_AVAILABLE kB"
echo "  free command: $SYSTEM_AVAILABLE kB"

# Check if values are within 1% of each other (memory can change quickly)
check_within_percent() {
    local val1=$1
    local val2=$2
    local percent=$3
    
    if [ $val1 -eq 0 ] || [ $val2 -eq 0 ]; then
        return 1
    fi
    
    local diff=$((val1 - val2))
    diff=${diff#-}  # absolute value
    local threshold=$((val1 * percent / 100))
    
    [ $diff -le $threshold ]
}

TESTS_PASSED=0
TOTAL_TESTS=3

if check_within_percent $OUR_TOTAL $SYSTEM_TOTAL 5; then
    echo "✅ Total memory matches (within 5%)"
    ((TESTS_PASSED++))
else
    echo "❌ Total memory differs significantly"
fi

if check_within_percent $OUR_FREE $SYSTEM_FREE 20; then
    echo "✅ Free memory matches (within 20%)"
    ((TESTS_PASSED++))
else
    echo "❌ Free memory differs significantly"
fi

if check_within_percent $OUR_AVAILABLE $SYSTEM_AVAILABLE 20; then
    echo "✅ Available memory matches (within 20%)"
    ((TESTS_PASSED++))
else
    echo "❌ Available memory differs significantly"
fi

if [ $TESTS_PASSED -eq $TOTAL_TESTS ]; then
    echo "✅ PASS: Memory verification passed ($TESTS_PASSED/$TOTAL_TESTS tests)"
    exit 0
else
    echo "❌ FAIL: Memory verification failed ($TESTS_PASSED/$TOTAL_TESTS tests passed)"
    exit 1
fi