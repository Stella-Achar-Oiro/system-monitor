#!/bin/bash
# Test script to verify user information matches who command

echo "=== USER VERIFICATION TEST ==="

# Get our implementation's user
OUR_USER=$(./simple_extractor system | grep "User:" | cut -d':' -f2 | tr -d ' ')

# Get system user using who
SYSTEM_USER=$(who am i | awk '{print $1}')
if [ -z "$SYSTEM_USER" ]; then
    SYSTEM_USER=$(whoami)
fi

echo "Our implementation: '$OUR_USER'"
echo "System command (who): '$SYSTEM_USER'"

if [ "$OUR_USER" = "$SYSTEM_USER" ]; then
    echo "✅ PASS: User verification matches"
    exit 0
else
    echo "❌ FAIL: User verification does not match"
    echo "Expected: '$SYSTEM_USER', Got: '$OUR_USER'"
    exit 1
fi