#!/bin/bash
# Test script to verify process count is reasonable compared to system commands

echo "=== PROCESS COUNT VERIFICATION TEST ==="

# Get our implementation's process count
OUR_PROCESSES=$(./simple_extractor system | grep "Total Processes:" | cut -d':' -f2 | tr -d ' ')

# Get system process count using different methods
PS_COUNT=$(ps aux | wc -l)
((PS_COUNT--))  # Remove header line

PROC_COUNT=$(ls /proc | grep '^[0-9]\+$' | wc -l)

# Try to get from top (timeout after 2 seconds)
TOP_COUNT=$(timeout 2s top -b -n1 | head -2 | tail -1 | awk '{print $2}' 2>/dev/null || echo "N/A")

echo "Our implementation: $OUR_PROCESSES"
echo "ps aux count: $PS_COUNT"
echo "/proc directory count: $PROC_COUNT"
echo "top command: $TOP_COUNT"

# Check if our count is within reasonable range (±20) of system counts
DIFF_PS=$((OUR_PROCESSES - PS_COUNT))
DIFF_PROC=$((OUR_PROCESSES - PROC_COUNT))

if [ ${DIFF_PS#-} -le 20 ] || [ ${DIFF_PROC#-} -le 20 ]; then
    echo "✅ PASS: Process count is within reasonable range"
    echo "Difference from ps: $DIFF_PS, from /proc: $DIFF_PROC"
    exit 0
else
    echo "❌ FAIL: Process count differs significantly from system commands"
    echo "Difference from ps: $DIFF_PS, from /proc: $DIFF_PROC"
    exit 1
fi