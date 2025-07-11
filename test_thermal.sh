#!/bin/bash
# Test script to verify thermal information matches system thermal sources

echo "=== THERMAL VERIFICATION TEST ==="

# Get our implementation's thermal info
OUR_OUTPUT=$(./simple_extractor thermal)

echo "Our thermal readings:"
echo "$OUR_OUTPUT"

# Test against various thermal sources
TESTS_PASSED=0
TOTAL_TESTS=0

# Test 1: ThinkPad thermal if available
if [ -f "/proc/acpi/ibm/thermal" ]; then
    ((TOTAL_TESTS++))
    echo ""
    echo "Testing ThinkPad thermal source..."
    
    SYSTEM_THINKPAD=$(cat /proc/acpi/ibm/thermal | awk '{print $2}')
    OUR_THINKPAD=$(echo "$OUR_OUTPUT" | grep "ThinkPad Thermal:" | awk '{print $3}' | sed 's/°C//')
    
    echo "System ThinkPad thermal: ${SYSTEM_THINKPAD}°C"
    echo "Our ThinkPad thermal: ${OUR_THINKPAD}°C"
    
    if [ "$OUR_THINKPAD" = "$SYSTEM_THINKPAD" ]; then
        echo "✅ ThinkPad thermal matches"
        ((TESTS_PASSED++))
    else
        if [ "$OUR_THINKPAD" = "Not" ]; then
            echo "❌ ThinkPad thermal not detected by our implementation"
        else
            echo "❌ ThinkPad thermal differs"
        fi
    fi
else
    echo "ℹ️  ThinkPad thermal source not available"
fi

# Test 2: Thermal zones
ZONE_TESTS=0
ZONE_PASSED=0

for zone in /sys/class/thermal/thermal_zone*/temp; do
    if [ -f "$zone" ]; then
        ((ZONE_TESTS++))
        zone_num=$(echo $zone | grep -o 'thermal_zone[0-9]*' | grep -o '[0-9]*')
        
        # Read system temperature (in millidegrees)
        system_temp_milli=$(cat $zone 2>/dev/null)
        if [ ! -z "$system_temp_milli" ] && [ "$system_temp_milli" -gt 0 ]; then
            system_temp=$((system_temp_milli / 1000))
            
            # Check our reading
            our_temp=$(echo "$OUR_OUTPUT" | grep "Thermal Zone $zone_num:" | awk '{print $4}' | sed 's/°C//')
            
            if [ ! -z "$our_temp" ] && [ "$our_temp" -gt 0 ]; then
                echo "Thermal Zone $zone_num - System: ${system_temp}°C, Ours: ${our_temp}°C"
                
                # Allow ±2 degrees difference
                diff=$((our_temp - system_temp))
                diff=${diff#-}  # absolute value
                
                if [ $diff -le 2 ]; then
                    ((ZONE_PASSED++))
                fi
            fi
        fi
    fi
done

if [ $ZONE_TESTS -gt 0 ]; then
    ((TOTAL_TESTS++))
    echo "Thermal zone test: $ZONE_PASSED/$ZONE_TESTS zones match (within 2°C)"
    
    if [ $ZONE_PASSED -gt 0 ]; then
        echo "✅ At least one thermal zone matches"
        ((TESTS_PASSED++))
    else
        echo "❌ No thermal zones match"
    fi
else
    echo "ℹ️  No thermal zones found"
fi

# Test 3: Hardware monitoring (hwmon)
HWMON_TESTS=0
HWMON_PASSED=0

for hwmon in /sys/class/hwmon/hwmon*/temp*_input; do
    if [ -f "$hwmon" ]; then
        ((HWMON_TESTS++))
        hwmon_num=$(echo $hwmon | grep -o 'hwmon[0-9]*' | grep -o '[0-9]*')
        
        # Read system temperature (in millidegrees)
        system_temp_milli=$(cat $hwmon 2>/dev/null)
        if [ ! -z "$system_temp_milli" ] && [ "$system_temp_milli" -gt 0 ]; then
            system_temp=$((system_temp_milli / 1000))
            
            # Check our reading
            our_temp=$(echo "$OUR_OUTPUT" | grep "Hwmon $hwmon_num:" | awk '{print $3}' | sed 's/°C//')
            
            if [ ! -z "$our_temp" ] && [ "$our_temp" -gt 0 ]; then
                echo "Hwmon $hwmon_num - System: ${system_temp}°C, Ours: ${our_temp}°C"
                
                # Allow ±2 degrees difference
                diff=$((our_temp - system_temp))
                diff=${diff#-}  # absolute value
                
                if [ $diff -le 2 ]; then
                    ((HWMON_PASSED++))
                fi
            fi
        fi
        
        # Only test first few hwmon sensors
        if [ $HWMON_TESTS -ge 3 ]; then
            break
        fi
    fi
done

if [ $HWMON_TESTS -gt 0 ]; then
    ((TOTAL_TESTS++))
    echo "Hardware monitoring test: $HWMON_PASSED/$HWMON_TESTS sensors match (within 2°C)"
    
    if [ $HWMON_PASSED -gt 0 ]; then
        echo "✅ At least one hwmon sensor matches"
        ((TESTS_PASSED++))
    else
        echo "❌ No hwmon sensors match"
    fi
else
    echo "ℹ️  No hwmon sensors found"
fi

# Overall assessment
if [ $TOTAL_TESTS -eq 0 ]; then
    echo "ℹ️  SKIP: No thermal sources available for testing"
    exit 0
elif [ $TESTS_PASSED -gt 0 ]; then
    echo "✅ PASS: Thermal verification passed ($TESTS_PASSED/$TOTAL_TESTS thermal sources match)"
    exit 0
else
    echo "❌ FAIL: Thermal verification failed (no thermal sources matched)"
    exit 1
fi