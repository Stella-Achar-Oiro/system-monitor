#!/bin/bash
# Test script to verify hostname matches hostname command

echo "=== HOSTNAME VERIFICATION TEST ==="

# Get our implementation's hostname
OUR_HOSTNAME=$(./simple_extractor system | grep "Hostname:" | cut -d':' -f2 | tr -d ' ')

# Get system hostname
SYSTEM_HOSTNAME=$(hostname)

echo "Our implementation: '$OUR_HOSTNAME'"
echo "System command (hostname): '$SYSTEM_HOSTNAME'"

if [ "$OUR_HOSTNAME" = "$SYSTEM_HOSTNAME" ]; then
    echo "✅ PASS: Hostname verification matches"
    exit 0
else
    echo "❌ FAIL: Hostname verification does not match"
    echo "Expected: '$SYSTEM_HOSTNAME', Got: '$OUR_HOSTNAME'"
    exit 1
fi