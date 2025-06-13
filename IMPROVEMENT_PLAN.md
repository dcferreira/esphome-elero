# ESPHome Elero Integration - Improvement Plan

## Problem Summary

The ESPHome Elero integration works most of the time but occasionally (intermittently) fails to control blinds:
- Commands appear to be sent successfully (blind shows "closing" state in HA)
- But blinds don't actually move
- Original Elero remote continues to work during these failures
- State eventually self-corrects after unknown duration (5 minutes to 5 hours)

## Root Cause Analysis

### Primary Suspected Causes
1. **RF Interference/Collision**: ESPHome and original remote signals interfering
2. **Command Counter Desynchronization**: Counter gets out of sync, blinds reject commands
3. **CC1101 Module State Issues**: Radio module stuck in wrong state
4. **RF Frequency Drift**: Transmission slightly off-frequency over time
5. **Power Supply Issues**: Insufficient power for consistent RF transmission

## Implementation Plan

### Phase 1: Enhanced Observability (Low Risk, High Insight)

#### 1.1 Add Comprehensive Transmission Logging
**Goal**: Track exactly what happens during each command transmission

**Changes Needed**:
- Log CC1101 state before/after each transmission attempt
- Add timing measurements for each transmission phase:
  - TX setup time
  - Actual transmission duration
  - TX completion time
- Log RSSI/LQI values from received acknowledgments
- Track correlation between transmission "success" and actual blind response
- Add detailed error logging when `wait_tx()` or `wait_tx_done()` fail

**Files to Modify**:
- `components/elero/elero.cpp` - `transmit()` function
- `components/elero/elero.cpp` - `send_command()` function
- `components/elero/cover/EleroCover.cpp` - `handle_commands()` function

#### 1.2 Create Debug Sensors
**Goal**: Make transmission statistics visible in Home Assistant

**New ESPHome Sensors to Add**:
- `transmission_success_rate` - Percentage over last 10/50/100 commands
- `average_response_time` - Time from command to state change
- `commands_sent_count` - Total commands sent since boot
- `commands_failed_count` - Total failed transmissions
- `silent_failures_count` - Commands sent successfully but blind didn't respond
- `last_rssi` / `last_lqi` - Signal quality from last received message
- `cc1101_state` - Current radio module state
- `last_successful_communication` - Timestamp of last successful command

**Implementation**:
- Add sensor platform to elero component
- Expose metrics via ESPHome sensor API
- Add Home Assistant dashboard cards for monitoring

#### 1.3 Enhanced State Change Tracking
**Goal**: Distinguish between "command sent" and "blind actually responded"

**Changes Needed**:
- Add timestamps for critical events:
  - `command_sent_time`
  - `state_change_detected_time`
  - `final_state_reached_time`
- Track expected vs actual state transitions
- Log when blinds get "stuck" in intermediate states
- Detect "silent failures" (transmission succeeded but no blind response)

**Files to Modify**:
- `components/elero/cover/EleroCover.cpp` - `set_rx_state()` function
- `components/elero/cover/EleroCover.cpp` - `start_movement()` function

### Phase 2: Smart Command Validation (Medium Risk, High Reliability)

#### 2.1 Implement Command Success Validation
**Goal**: Detect when commands were sent but blinds didn't respond

**Logic to Implement**:
- After sending CLOSE command, expect state change to CLOSING within 10 seconds
- After sending OPEN command, expect state change to OPENING within 10 seconds
- After sending STOP command, expect state change to IDLE within 5 seconds
- If no expected state change detected, mark command as "failed" even if transmission succeeded
- Track these "silent failures" separately from transmission failures
- Add configurable timeout values per command type

**New Configuration Options**:
```yaml
cover:
  - platform: elero
    # ... existing config ...
    command_response_timeout: 10s  # How long to wait for blind to respond
    state_change_timeout: 5s       # How long to wait for initial state change
```

#### 2.2 Add Aggressive State Polling During Critical Windows
**Goal**: Catch state changes faster and detect failures sooner

**Changes Needed**:
- Current polling: Every 5 minutes normally, every 2 seconds while moving
- New polling strategy:
  - Poll every 1 second for 15 seconds after sending ANY command
  - Poll every 2 seconds while in moving states (existing behavior)
  - Return to normal 5-minute polling once expected state change is confirmed
  - Add timeout detection: if still in intermediate state after expected duration + 30 seconds, consider it stuck

**Files to Modify**:
- `components/elero/cover/EleroCover.cpp` - `loop()` function
- Add new polling state machine

#### 2.3 Implement Smart Retry Logic
**Goal**: Retry not just transmission failures, but also "silent failures"

**Current Behavior**: Only retry if `send_command()` returns false
**New Strategy**:
- Also retry if expected state change doesn't occur within timeout
- Add exponential backoff: wait 5s, then 10s, then 20s between retries
- Maximum 3 total retries before giving up and alerting
- Different retry strategies for different failure types:
  - Transmission failure: immediate retry with short delay
  - Silent failure: longer delay before retry (avoid interference)
  - Counter desync: try incrementing counter before retry

**New States to Track**:
- `COMMAND_PENDING` - Command sent, waiting for response
- `COMMAND_FAILED_SILENT` - No transmission error but no blind response
- `COMMAND_FAILED_TRANSMISSION` - RF transmission failed
- `COMMAND_SUCCEEDED` - Blind responded as expected

### Phase 3: RF Communication Improvements (Medium Risk, Medium Impact)

#### 3.1 Add Pre-transmission RF Environment Check
**Goal**: Avoid transmitting during RF interference

**Implementation**:
- Before transmitting, listen for ongoing RF activity
- Wait for "quiet period" (no received packets for X milliseconds)
- Add configurable quiet period duration
- Add random jitter (0-500ms) to transmission timing to avoid systematic collisions
- Skip environment check for urgent commands (STOP)

**New Configuration Options**:
```yaml
elero:
  # ... existing config ...
  quiet_period_ms: 100        # Wait for quiet RF before transmitting
  transmission_jitter_ms: 500 # Random delay to avoid collisions
  skip_rf_check_for_stop: true # Don't wait for STOP commands
```

#### 3.2 Implement Command Counter Recovery
**Goal**: Handle counter desynchronization gracefully

**Detection Logic**:
- If 3+ consecutive commands fail (both transmission and silent failures)
- Try resetting counter to last known good value + 1
- Try incrementing counter by larger amounts (simulate missed commands)

**Recovery Strategies**:
1. **Counter Reset**: Set counter to 1 and try again
2. **Counter Jump**: Increment counter by 10 and try again
3. **Counter Sync**: Send check command to get current counter from blind

**New Home Assistant Services**:
- `elero.reset_counter` - Manually reset command counter for a blind
- `elero.sync_counter` - Attempt to sync counter with blind

#### 3.3 Enhanced Error Recovery
**Goal**: Recover from CC1101 module getting stuck in bad states

**Health Check Implementation**:
- Periodic CC1101 module health check (every 10 minutes)
- Check if module responds to status requests
- Verify module is in expected state (RX when idle)
- Count consecutive transmission failures

**Recovery Actions**:
- If health check fails: reinitialize CC1101 module
- If 10+ consecutive transmission failures: full module reset
- If module state is unexpected: force state transition

**New Home Assistant Services**:
- `elero.reset_radio` - Force CC1101 module reset
- `elero.radio_health_check` - Manual health assessment

### Phase 4: Advanced Features (Higher Risk, Nice-to-Have)

#### 4.1 Additional Home Assistant Services for Debugging
```yaml
# New services to implement
elero.test_communication:
  description: Send test command and report detailed results
  fields:
    entity_id: { required: true }
    command: { required: false, default: "check" }

elero.force_state_sync:
  description: Poll all blinds and update HA states immediately
  
elero.get_rf_statistics:
  description: Return detailed RF transmission statistics

elero.set_debug_mode:
  description: Enable/disable verbose debugging for specific blind
  fields:
    entity_id: { required: true }
    enabled: { required: true }
```

#### 4.2 Implement RF Quality Monitoring
**Features**:
- Continuous background monitoring of 868MHz band
- Track interference patterns (time of day, duration, intensity)
- Alert when interference levels exceed threshold
- Automatic transmission scheduling during "quiet" periods
- Integration with Home Assistant for interference alerts

#### 4.3 Advanced Analytics and Reporting
**Metrics to Track**:
- Success rate by time of day/day of week
- Correlation between weather and RF performance
- Blind-specific reliability statistics
- RF environment quality over time

**Reporting Features**:
- Weekly reliability reports
- Performance degradation alerts
- Predictive failure detection based on trends

## Implementation Priority

### Phase 1 (Immediate - Week 1-2)
**Why First**: Zero risk, maximum insight
- Start with 1.1 and 1.3 (logging and state tracking)
- Then add 1.2 (debug sensors)
- Essential for understanding current behavior

### Phase 2 (Short Term - Week 3-4)
**Why Second**: Addresses core reliability issues with low risk
- Builds on observability from Phase 1
- 2.1 first (command validation), then 2.2 (better polling), finally 2.3 (smart retry)
- Should solve majority of reliability issues

### Phase 3 (Medium Term - Week 5-8)
**Why Third**: Higher risk changes to RF logic
- Need data from Phases 1-2 to validate changes work
- 3.1 and 3.3 first (safer changes), then 3.2 (counter recovery)
- Some features might not be needed if earlier phases solve the problem

### Phase 4 (Long Term - As Needed)
**Why Last**: Nice-to-have features for ongoing maintenance
- Only implement if earlier phases don't fully solve reliability issues
- Focus on most useful debugging features first

## Success Metrics

### Phase 1 Success Criteria
- Can distinguish between transmission failures and silent failures
- Have visibility into RF communication quality
- Can track blind response times in Home Assistant

### Phase 2 Success Criteria
- Silent failures are detected and retried automatically
- Overall reliability improves to >95% success rate
- Stuck blinds are detected and handled gracefully

### Phase 3 Success Criteria
- RF interference-related failures are minimized
- Counter desynchronization is rare and auto-recoverable
- System is resilient to temporary RF issues

### Overall Success
- Blind control reliability matches or exceeds original remote
- Failures are predictable, logged, and auto-recovered
- System provides clear feedback when issues occur

## Configuration Changes Required

### Enhanced Logging Configuration
```yaml
logger:
  level: INFO
  logs:
    elero: DEBUG
    elero.cover: DEBUG
    
# Optional verbose mode for troubleshooting
# logger:
#   level: DEBUG
#   logs:
#     elero: VERBOSE
#     elero.cover: VERBOSE
```

### New Cover Configuration Options
```yaml
cover:
  - platform: elero
    # ... existing required config ...
    
    # New reliability options
    command_response_timeout: 10s      # How long to wait for blind response
    state_change_timeout: 5s           # How long to wait for state change  
    max_retries: 3                     # Maximum command retries
    retry_backoff_base: 5s             # Base retry delay (exponential backoff)
    
    # New RF options  
    quiet_period_ms: 100               # Wait for RF quiet before transmitting
    transmission_jitter_ms: 500        # Random transmission delay
    skip_rf_check_for_stop: true       # Don't delay STOP commands
    
    # New monitoring options
    enable_debug_sensors: false        # Create debug sensors in HA
    health_check_interval: 10min       # CC1101 health check frequency
```

## Notes for Implementation

### Testing Strategy
- Implement Phase 1 first and collect data for 1-2 weeks
- Use collected data to validate assumptions before Phase 2
- A/B test new features against current behavior when possible
- Keep detailed logs of reliability improvements

### Backwards Compatibility
- All new features should be opt-in via configuration
- Existing configurations should continue to work unchanged
- New sensors and services are additive only

### Performance Considerations
- Additional logging and monitoring will increase CPU usage slightly
- Debug sensors should be optional (disabled by default)
- Aggressive polling increases RF traffic - balance between responsiveness and interference

### Rollback Plan
- Each phase should be implementable as separate commits
- Features should be toggleable via configuration
- Keep ability to revert to "classic" behavior if needed 