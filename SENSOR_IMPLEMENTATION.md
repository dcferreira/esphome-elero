# Elero Debug Sensor Platform

## Overview

A clean sensor platform has been implemented to track the 4 most critical debug metrics for diagnosing intermittent Elero blind control failures.

## Available Sensors

### 1. Silent Failures Count
- **Tracks**: Commands sent successfully (RF transmission OK) but blind doesn't respond
- **Key Metric**: This is the primary indicator of the reported intermittent issue
- **Unit**: Count (total increasing)
- **Timeout**: 10 seconds - if no response after this time, counted as silent failure

### 2. Success Rate  
- **Tracks**: Overall percentage of commands that get responses
- **Formula**: `((commands_sent - commands_failed) / commands_sent) * 100`
- **Unit**: Percentage
- **Updates**: After every command and response

### 3. Average Response Time
- **Tracks**: Average time from command transmission to blind response
- **Unit**: Milliseconds  
- **Updates**: Each time a response is received
- **Use**: Helps identify communication latency issues

### 4. Last RSSI
- **Tracks**: Signal strength of the most recently received message
- **Unit**: dBm
- **Updates**: Every time a message is received from any blind
- **Use**: Monitor RF signal quality

## Architecture

### Files Added:
- `components/elero/sensor/__init__.py` - ESPHome sensor platform configuration
- `components/elero/sensor/EleroSensor.h` - C++ sensor class header
- `components/elero/sensor/EleroSensor.cpp` - C++ sensor class implementation

### Files Modified:
- `components/elero/elero.h` - Added sensor support and metric tracking
- `components/elero/elero.cpp` - Added sensor update logic
- `components/elero/cover/EleroCover.h` - Added silent failure detection variables
- `components/elero/cover/EleroCover.cpp` - Added response tracking and timeout logic

## Usage Example

```yaml
sensor:
  - platform: elero
    elero_id: elero_transceiver
    
    silent_failures:
      name: "Elero Silent Failures"
      
    success_rate:
      name: "Elero Success Rate"
      
    average_response_time:
      name: "Elero Avg Response Time" 
      
    last_rssi:
      name: "Elero Last RSSI"
```

## Detection Logic

### Silent Failure Detection
1. Track when action commands (UP/DOWN/STOP/TILT) are sent (CHECK commands ignored)
2. Wait for response message from blind with matching state change
3. If no response after 10 seconds → silent failure detected
4. LOG: "SILENT FAILURE: [command] sent to blind [addr] but no response after [time]ms"

### Response Time Tracking
1. Timestamp when command sent
2. Calculate elapsed time when response received
3. Add to running average
4. LOG: "STATE RX: [state] from blind [addr] (response_time=[time]ms)"

## Benefits

1. **Clean Architecture**: Proper ESPHome sensor platform, not mixed into main component
2. **Focused Metrics**: Just the 4 most important metrics for debugging the specific issue
3. **Home Assistant Integration**: Sensors appear as entities with history/graphing
4. **Optional**: Sensors can be omitted if not needed
5. **Maintainable**: Clear separation of concerns

## Debugging the Original Issue

The **Silent Failures** sensor is the key metric for diagnosing the reported problem:
- Should normally be 0 or very low
- Spikes indicate RF transmission succeeded but blind didn't respond  
- Combined with RSSI can indicate if it's signal strength related
- Response time can show if communication is getting slower before failures 