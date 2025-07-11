#!/bin/bash
# Test script to verify network information matches /proc/net/dev and ifconfig

echo "=== NETWORK VERIFICATION TEST ==="

# Get our implementation's network info
OUR_OUTPUT=$(./simple_extractor network)

# Test against /proc/net/dev directly
echo "Comparing with /proc/net/dev..."

# Check if we detect the same interfaces
OUR_INTERFACES=$(echo "$OUR_OUTPUT" | grep "Interface:" | awk '{print $2}' | sort)
PROC_INTERFACES=$(tail -n +3 /proc/net/dev | awk -F: '{print $1}' | tr -d ' ' | sort)

echo "Our interfaces: $(echo $OUR_INTERFACES | tr '\n' ' ')"
echo "Proc interfaces: $(echo $PROC_INTERFACES | tr '\n' ' ')"

# Count matching interfaces
MATCHING_INTERFACES=0
TOTAL_INTERFACES=0

for iface in $PROC_INTERFACES; do
    ((TOTAL_INTERFACES++))
    if echo "$OUR_INTERFACES" | grep -q "^$iface$"; then
        ((MATCHING_INTERFACES++))
    else
        echo "❌ Missing interface: $iface"
    fi
done

echo "Interface detection: $MATCHING_INTERFACES/$TOTAL_INTERFACES interfaces detected"

# Test specific interface data (use first interface from /proc/net/dev)
FIRST_INTERFACE=$(echo $PROC_INTERFACES | awk '{print $1}')
if [ ! -z "$FIRST_INTERFACE" ]; then
    echo ""
    echo "Testing data for interface: $FIRST_INTERFACE"
    
    # Get our data for this interface
    OUR_RX=$(echo "$OUR_OUTPUT" | awk "/Interface: $FIRST_INTERFACE/,/^$/" | grep "RX Bytes:" | awk '{print $3}')
    OUR_TX=$(echo "$OUR_OUTPUT" | awk "/Interface: $FIRST_INTERFACE/,/^$/" | grep "TX Bytes:" | awk '{print $3}')
    
    # Get system data from /proc/net/dev
    PROC_LINE=$(grep "^\s*$FIRST_INTERFACE:" /proc/net/dev)
    PROC_RX=$(echo $PROC_LINE | awk '{print $2}')
    PROC_TX=$(echo $PROC_LINE | awk '{print $10}')
    
    echo "RX Bytes - Our: $OUR_RX, Proc: $PROC_RX"
    echo "TX Bytes - Our: $OUR_TX, Proc: $PROC_TX"
    
    RX_MATCH=false
    TX_MATCH=false
    
    if [ "$OUR_RX" = "$PROC_RX" ]; then
        echo "✅ RX bytes match exactly"
        RX_MATCH=true
    else
        echo "❌ RX bytes differ"
    fi
    
    if [ "$OUR_TX" = "$PROC_TX" ]; then
        echo "✅ TX bytes match exactly"
        TX_MATCH=true
    else
        echo "❌ TX bytes differ"
    fi
fi

# Test IP address detection (compare with ip command if available)
echo ""
echo "Testing IP address detection..."
if command -v ip >/dev/null 2>&1; then
    # Get IP addresses from ip command
    IP_ADDRESSES=$(ip addr show | grep "inet " | grep -v "127.0.0.1" | awk '{print $2}' | cut -d'/' -f1 | head -1)
    OUR_IPS=$(echo "$OUR_OUTPUT" | grep "IP Address:" | awk '{print $3}' | grep -v "127.0.0.1" | head -1)
    
    if [ ! -z "$IP_ADDRESSES" ] && [ ! -z "$OUR_IPS" ]; then
        if echo "$OUR_IPS" | grep -q "$IP_ADDRESSES"; then
            echo "✅ IP address detection working"
        else
            echo "❌ IP address detection differs"
            echo "System: $IP_ADDRESSES"
            echo "Ours: $OUR_IPS"
        fi
    else
        echo "ℹ️  IP address comparison skipped (no addresses found)"
    fi
else
    echo "ℹ️  IP address test skipped (ip command not available)"
fi

# Overall assessment
TESTS_PASSED=0
TOTAL_TESTS=2

if [ $MATCHING_INTERFACES -eq $TOTAL_INTERFACES ]; then
    echo "✅ Interface detection test passed"
    ((TESTS_PASSED++))
fi

if [ "$RX_MATCH" = "true" ] && [ "$TX_MATCH" = "true" ]; then
    echo "✅ Data parsing test passed"
    ((TESTS_PASSED++))
fi

if [ $TESTS_PASSED -eq $TOTAL_TESTS ]; then
    echo "✅ PASS: Network verification passed ($TESTS_PASSED/$TOTAL_TESTS tests)"
    exit 0
else
    echo "❌ FAIL: Network verification failed ($TESTS_PASSED/$TOTAL_TESTS tests passed)"
    exit 1
fi