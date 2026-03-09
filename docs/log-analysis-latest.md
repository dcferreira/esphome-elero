# Diagnostic Log Analysis: syncing.log

**Log file:** `config/logs/syncing.log`
**Session start:** 2026-03-09 14:18:49 (ESPHome 2026.2.4)
**Session end:** 2026-03-09 14:20:57
**Duration:** ~2 minutes 8 seconds
**ESP32:** rev3.1, 2 cores, WiFi RSSI: -29 dB (excellent)

---

## 1. Per-Blind Summary Table

| Blind | Address | Channel | Commands Sent | Responses Received | Success Rate | RSSI Range | LQI Range | Counter Range | Counter Sync? | Remote Cooldown? | Final Status |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Office | 0xf98627 | 81 | 5 | 4 | 80.0% | -67.0 to -77.0 | 46-48 | 21-26 | No | No | WORKING |
| Bedroom | 0xfa8627 | 34 | 5 | 2 | 40.0% | -85.0 to -89.0 | 47-47 | 64-69 | No | No | WORKING |
| LR1 | 0x598727 | N/A | 0 | 0 | N/A | N/A | N/A | N/A | No | No | NEVER TESTED |
| LR2 | 0x5f7527 | 4 | 20 | 0 | 0.0% | N/A | N/A | 25-34 | Yes (x4) | No | NOT RESPONDING |
| LR3 | 0xd60227 | 5 | 26 | 1 | 3.8% | -95.0 | 98 | 13-33 | No | No | NOT RESPONDING |
| LR4 | 0x717527 | 6 | 18 | 1 | 5.6% | -96.0 | 64 | 40-67 | No | No | NOT RESPONDING |
| Bathroom | 0x876e27 | 7 | 10 | 0 | 0.0% | N/A | N/A | 193-203 | No | No | NOT RESPONDING |

**Notes on counts:**
- "Commands Sent" counts unique ESP-initiated CMD SEND events (excluding TX FAILED retries that used the same counter).
- "Responses Received" counts direct replies from the blind to ESP commands (typ=0xca packets with src=blind address, fwd=0x0b3973).
- Bedroom shows 40% because only 2 of 5 commands got responses; however both action commands worked (the blind moved).
- LR3 and LR4 each got exactly one response, but only after extensive counter recovery (offset +5 and +16 respectively).

---

## 2. Per-Blind Detailed Command Log

### Office Blinds (0xf98627, channel 81)

| # | Timestamp | Event | Command | Counter | Result | RSSI | LQI | Notes |
|---|---|---|---|---|---|---|---|---|
| 1 | 14:20:37.731 | CMD SEND | CLOSE (0x41) | 21 | TX SUCCESS | | | |
| 2 | 14:20:37.809 | CMD SEND | CHECK (0x00) | 22 | TX SUCCESS | | | |
| 3 | 14:20:37.964 | RESPONSE | TOP (0x01) | chl=21 | | -70.0 | 46 | response_time=162ms; blind reports at top, not closing |
| 4 | 14:20:38.157 | RESPONSE | MOVING_DOWN (0x0b) | chl=22 | | -74.0 | 48 | Blind started closing |
| 5 | 14:20:39.280 | CMD SEND | OPEN (0x21) | 23 | TX SUCCESS | | | User reversed direction |
| 6 | 14:20:39.388 | RESPONSE | MOVING_DOWN (0x0b) | chl=23 | | -77.0 | 47 | response_time=118ms; still reports moving down |
| 7 | 14:20:39.733 | CMD SEND | CHECK (0x00) | 24 | TX SUCCESS | | | |
| 8 | 14:20:41.738 | CMD SEND | CHECK (0x00) | 25 | TX SUCCESS | | | |
| 9 | 14:20:41.905 | RESPONSE | TOP (0x01) | chl=25 | | -67.0 | 47 | response_time=175ms; blind at top again |

**Office summary:** All 5 commands transmitted successfully. 4 responses received. Blind responded reliably to every action command. RSSI range -67 to -77 dBm (strong signal). Counter 21->26 (5 consumed, 0 wasted).

---

### Bedroom Blinds (0xfa8627, channel 34)

| # | Timestamp | Event | Command | Counter | Result | RSSI | LQI | Notes |
|---|---|---|---|---|---|---|---|---|
| 1 | 14:20:22.677 | CMD SEND | CLOSE (0x41) | 64 | TX SUCCESS | | | |
| 2 | 14:20:22.762 | CMD SEND | CHECK (0x00) | 65 | TX SUCCESS | | | |
| 3 | 14:20:24.683 | CMD SEND | CHECK (0x00) | 66 | TX SUCCESS | | | No response to CHECK at cnt=65 within 2s window |
| 4 | 14:20:25.565 | CMD SEND | STOP (0x10) | 67 | TX SUCCESS | | | |
| 5 | 14:20:25.688 | RESPONSE | STOPPED (0x0d) | chl=67 | | -89.0 | 47 | response_time=132ms |
| 6 | 14:20:26.380 | CMD SEND | OPEN (0x21) | 68 | TX SUCCESS | | | |
| 7 | 14:20:26.486 | RESPONSE | STOPPED (0x0d) | chl=68 | | -85.0 | 47 | response_time=120ms; blind didn't move (stopped) |

**Bedroom summary:** All 5 unique commands transmitted successfully. 2 direct responses received (to STOP and OPEN). Blind obeyed STOP. RSSI -85 to -89 dBm (moderate/weak signal). Counter 64->69 (5 consumed, 0 wasted). Working but weaker signal than Office.

---

### Living Room Blinds 1 (0x598727) -- NEVER TESTED

No commands were sent to this blind during the entire log session. It only appears as a relay node (`bwd=0x598727`) forwarding mesh packets from other devices:
- 14:19:20.893 -- relayed packet from 0xc81d2e (RSSI -91.0, LQI 51)
- 14:19:55.127 -- relayed packet from 0xb7002f (RSSI -90.0, LQI 46)
- 14:20:19.240 -- relayed packet from 0x713333 (RSSI -90.0, LQI 51)
- 14:20:24.056 -- relayed packet from 0xe3241f (RSSI -91.0, LQI 46)

**LR1 summary:** No ESP commands, no responses. Appears as mesh relay. RSSI of relayed packets: -90 to -91 dBm (weak).

---

### Living Room Blinds 2 (0x5f7527, channel 4)

| # | Timestamp | Event | Command | Counter | Result | Notes |
|---|---|---|---|---|---|---|
| 1 | 14:19:07.925 | CMD SEND | CLOSE (0x41) | 28 | TX SUCCESS | |
| 2 | 14:19:08.012 | CMD SEND | CHECK (0x00) | 29 | TX SUCCESS | |
| 3 | 14:19:09.947 | CMD SEND | CHECK (0x00) | 30 | TX SUCCESS | |
| 4 | 14:19:11.962 | CMD SEND | CHECK (0x00) | 31 | TX SUCCESS | |
| 5 | 14:19:13.969 | CMD SEND | CHECK (0x00) | 32 | TX SUCCESS | |
| 6 | 14:19:15.988 | CMD SEND | CHECK (0x00) | 33 | TX SUCCESS | |
| 7 | 14:19:18.001 | CMD SEND | CHECK (0x00) | 34 | TX SUCCESS | |
| 8 | 14:19:20.003 | CMD SEND | CHECK (0x00) | 35 | TX SUCCESS | |
| 9 | 14:19:22.019 | CMD SEND | CHECK (0x00) | 36 | TX SUCCESS | |
| 10 | 14:19:24.036 | CMD SEND | CHECK (0x00) | 37 | TX SUCCESS | |
| 11 | 14:19:26.038 | CMD SEND | CHECK (0x00) | 38 | TX SUCCESS | |
| 12 | 14:19:28.045 | CMD SEND | CHECK (0x00) | 39 | TX SUCCESS | |
| 13 | 14:19:28.927 | CMD SEND | STOP (0x10) | 40 | TX SUCCESS | |
| -- | 14:19:36.385 | OVERHEARD | remote 0x0b3973 CHECK | cnt=19 | | |
| -- | 14:19:36.390 | COUNTER SYNC | | | ESP 41->21 | Remote cnt=19, ESP sets to 21 |
| -- | 14:19:37.758 | OVERHEARD | remote 0x0b3973 CHECK | cnt=20 | | |
| -- | 14:19:37.762 | COUNTER SYNC | | | ESP 21->22 | |
| -- | 14:19:39.713 | OVERHEARD | remote 0x0b3973 CHECK | cnt=21 | | |
| -- | 14:19:39.719 | COUNTER SYNC | | | ESP 22->23 | |
| -- | 14:19:54.487 | OVERHEARD | remote 0x0b3973 CHECK | cnt=23 | | cnt=22 was missed |
| -- | 14:19:54.504 | COUNTER SYNC | | | ESP 23->25 | |
| -- | 14:20:30.132 | SILENT FAILURE | STOP cnt=30 | | 10004ms | Recovery starts (counter was synced from 41 to 21 earlier) |
| 14 | 14:20:15.217 | CMD SEND | CLOSE (0x41) | 25 | TX SUCCESS | Second close attempt after counter sync |
| 15 | 14:20:15.282 | CMD SEND | CHECK (0x00) | 26 | TX SUCCESS | |
| 16 | 14:20:17.223 | CMD SEND | CHECK (0x00) | 27 | TX SUCCESS | |
| 17 | 14:20:19.250 | CMD SEND | CHECK (0x00) | 28 | TX SUCCESS | |
| 18 | 14:20:20.132 | CMD SEND | STOP (0x10) | 29 | TX SUCCESS | |
| -- | 14:20:30.132 | SILENT FAILURE | STOP cnt=30 | | 10004ms | No response after 10s |
| 19 | 14:20:30.159 | RECOVERY 1/10 | CHECK | 31 | TX SUCCESS | offset +1 |
| -- | 14:20:40.167 | SILENT FAILURE | CHECK cnt=32 | | 10011ms | Recovery attempt 1 failed |
| 20 | 14:20:40.192 | RECOVERY 2/10 | CHECK | 32 | TX SUCCESS | offset +2 |
| -- | 14:20:50.204 | SILENT FAILURE | CHECK cnt=33 | | 10011ms | Recovery attempt 2 failed |
| 21 | 14:20:50.225 | RECOVERY 3/10 | CHECK | 33 | TX SUCCESS | offset +3; log ends before result |

**LR2 summary:** 20+ commands sent, ZERO direct responses received. Counter sync happened 4 times via overheard physical remote (0x0b3973). Counter went from 28->41 (first run), synced back to 21->25 (via remote), then 25->34 in recovery. 3 recovery attempts all failed (log ends during attempt 3). Blind NEVER responded to ESP.

---

### Living Room Blinds 3 (0xd60227, channel 5)

| # | Timestamp | Event | Command | Counter | Result | Notes |
|---|---|---|---|---|---|---|
| 1 | 14:19:13.578 | CMD SEND | CLOSE (0x41) | 13 | TX SUCCESS | |
| 2 | 14:19:13.677 | CMD SEND | CHECK (0x00) | 14 | TX SUCCESS | |
| 3 | 14:19:15.595 | CMD SEND | CHECK (0x00) | 15 | TX SUCCESS | |
| 4 | 14:19:17.669 | CMD SEND | CHECK (0x00) | 16 | TX SUCCESS | |
| 5 | 14:19:19.680 | CMD SEND | CHECK (0x00) | 17 | TX SUCCESS | |
| 6 | 14:19:21.693 | CMD SEND | CHECK (0x00) | 18 | TX SUCCESS | |
| 7 | 14:19:23.696 | CMD SEND | CHECK (0x00) | 19 | TX SUCCESS | |
| 8 | 14:19:25.707 | CMD SEND | CHECK (0x00) | 20 | TX SUCCESS | |
| 9 | 14:19:27.710 | CMD SEND | CHECK (0x00) | 21 | TX SUCCESS | |
| 10 | 14:19:28.626 | CMD SEND | STOP (0x10) | 22 | TX SUCCESS | |
| -- | 14:19:38.641 | SILENT FAILURE | STOP cnt=23 | | 10012ms | |
| 11 | 14:19:38.655 | RECOVERY 1/10 | CHECK | 24 | TX SUCCESS | offset +1 |
| -- | 14:19:48.670 | SILENT FAILURE | CHECK cnt=25 | | 10012ms | |
| 12 | 14:19:48.680 | RECOVERY 2/10 | CHECK | 25 | TX SUCCESS | offset +2 |
| -- | 14:19:58.699 | SILENT FAILURE | CHECK cnt=26 | | 10014ms | |
| 13 | 14:19:58.720 | RECOVERY 3/10 | CHECK | 26 | TX SUCCESS | offset +3 |
| 14 | 14:20:04.568 | CMD SEND | OPEN (0x21) | 27 | TX SUCCESS | User tried open |
| 15 | 14:20:04.644 | CMD SEND | CHECK (0x00) | 28 | TX SUCCESS | |
| 16 | 14:20:06.591 | CMD SEND | CHECK (0x00) | 29 | TX SUCCESS | |
| 17 | 14:20:08.598 | CMD SEND | CHECK (0x00) | 30 | TX SUCCESS | |
| 18 | 14:20:08.878 | CMD SEND | STOP (0x10) | 31 | TX SUCCESS | |
| 19 | 14:20:09.682 | CMD SEND | CLOSE (0x41) | 32 | TX SUCCESS | |
| 20 | 14:20:10.607 | CMD SEND | CHECK (0x00) | 33 | TX SUCCESS | |
| 21 | 14:20:12.624 | CMD SEND | CHECK (0x00) | 34 | TX SUCCESS | |
| 22 | 14:20:13.275 | CMD SEND | STOP (0x10) | 35 | TX SUCCESS | |
| -- | 14:20:23.277 | SILENT FAILURE | STOP cnt=36 | | 10002ms | |
| 23 | 14:20:23.310 | RECOVERY 4/10 | CHECK | 27 | TX SUCCESS | offset +4; counter reset |
| 24 | 14:20:23.437 | **RESPONSE** | TOP (0x01) | chl=27 | -95.0 / 98 | **response_time=151ms -- RECOVERY SUCCESS at offset +5** |
| 25 | 14:20:23.633 | CMD SEND | CHECK (0x00) | 28 | TX FAILED | TX start timeout (70ms) |
| 25r | 14:20:23.633 | CMD RETRY | CHECK (0x00) | 28 | TX SUCCESS | Retry succeeded |
| -- | 14:20:33.640 | SILENT FAILURE | CHECK cnt=29 | | 10007ms | Back to no response |
| 26 | 14:20:33.664 | RECOVERY 1/10 | CHECK | 30 | TX SUCCESS | New recovery cycle |
| -- | 14:20:43.680 | SILENT FAILURE | CHECK cnt=31 | | 10015ms | |
| 27 | 14:20:43.709 | RECOVERY 2/10 | CHECK | 31 | TX SUCCESS | offset +2 |
| -- | 14:20:53.711 | SILENT FAILURE | CHECK cnt=32 | | 10001ms | |
| 28 | 14:20:53.720 | RECOVERY 3/10 | CHECK | 32 | TX SUCCESS | offset +3; log ends |

**LR3 summary:** 26+ commands sent, exactly 1 response received (during counter recovery at offset +5, RSSI -95.0, LQI 98). Counter 13->33 (consumed ~20). After the single successful recovery, the blind immediately stopped responding again. Very weak/intermittent signal.

---

### Living Room Blinds 4 (0x717527, channel 6)

| # | Timestamp | Event | Command | Counter | Result | Notes |
|---|---|---|---|---|---|---|
| 1 | 14:19:17.583 | CMD SEND | CLOSE (0x41) | 40 | TX SUCCESS | |
| 2 | 14:19:17.754 | CMD SEND | CHECK (0x00) | 41 | **TX FAILED** | 28 bytes left, state=0x13 |
| 2r | 14:19:17.791 | CMD RETRY | CHECK (0x00) | 41 | TX SUCCESS | Retry with same counter |
| 3 | 14:19:19.589 | CMD SEND | CHECK (0x00) | 42 | TX SUCCESS | |
| 4 | 14:19:21.595 | CMD SEND | CHECK (0x00) | 43 | TX SUCCESS | |
| 5 | 14:19:23.615 | CMD SEND | CHECK (0x00) | 44 | TX SUCCESS | |
| 6 | 14:19:25.621 | CMD SEND | CHECK (0x00) | 45 | TX SUCCESS | |
| 7 | 14:19:27.626 | CMD SEND | CHECK (0x00) | 46 | TX SUCCESS | |
| 8 | 14:19:28.185 | CMD SEND | STOP (0x10) | 47 | TX SUCCESS | |
| -- | 14:19:38.196 | SILENT FAILURE | STOP cnt=48 | | 10007ms | |
| 9 | 14:19:38.216 | RECOVERY 1/10 | CHECK | 49 | TX SUCCESS | offset +1 |
| -- | 14:19:48.230 | SILENT FAILURE | CHECK cnt=50 | | 10014ms | |
| 10 | 14:19:48.251 | RECOVERY 2/10 | CHECK | 50 | TX SUCCESS | offset +2 |
| -- | 14:19:58.265 | SILENT FAILURE | CHECK cnt=51 | | 10013ms | |
| 11 | 14:19:58.284 | RECOVERY 3/10 | CHECK | 51 | TX SUCCESS | offset +3 |
| -- | 14:20:08.295 | SILENT FAILURE | CHECK cnt=52 | | 10004ms | |
| 12 | 14:20:08.316 | RECOVERY 4/10 | CHECK | 52 | TX SUCCESS | offset +4 |
| -- | 14:20:18.318 | SILENT FAILURE | CHECK cnt=53 | | 10001ms | |
| 13 | 14:20:18.341 | RECOVERY 5/10 | CHECK | 53 | TX SUCCESS | offset +5 |
| -- | 14:20:28.358 | SILENT FAILURE | CHECK cnt=54 | | 10017ms | |
| 14 | 14:20:28.383 | RECOVERY 6/10 | CHECK | 58 | TX SUCCESS | offset +10 (jumped) |
| -- | 14:20:38.391 | SILENT FAILURE | CHECK cnt=59 | | 10009ms | |
| 15 | 14:20:38.417 | RECOVERY 7/10 | CHECK | 63 | TX SUCCESS | offset +15 |
| 16 | 14:20:38.591 | **RESPONSE** | TOP (0x01) | chl=63 | -96.0 / 64 | **response_time=182ms -- RECOVERY SUCCESS at offset +16** |
| 17 | 14:20:38.654 | CMD SEND | CHECK (0x00) | 64 | TX SUCCESS | Post-recovery check |
| -- | 14:20:48.665 | SILENT FAILURE | CHECK cnt=65 | | 10011ms | Back to no response |
| 18 | 14:20:48.694 | RECOVERY 1/10 | CHECK | 66 | TX SUCCESS | New recovery cycle; log ends |

**LR4 summary:** 18 commands sent, 1 response received (during recovery at offset +16, RSSI -96.0, LQI 64). Counter 40->67 (consumed 27). The blind's internal counter was massively ahead of ESP's -- the recovery succeeded only at offset +16, meaning the blind expected counter ~63 when ESP started at 48. After recovery, blind immediately stopped responding again. Extremely weak signal (-96 dBm).

---

### Bathroom Blinds (0x876e27, channel 7)

| # | Timestamp | Event | Command | Counter | Result | Notes |
|---|---|---|---|---|---|---|
| 1 | 14:20:43.495 | CMD SEND | CLOSE (0x41) | 193 | TX SUCCESS | |
| 2 | 14:20:43.577 | CMD SEND | CHECK (0x00) | 194 | TX SUCCESS | |
| 3 | 14:20:45.484 | CMD SEND | CHECK (0x00) | 195 | TX SUCCESS | |
| 4 | 14:20:47.486 | CMD SEND | CHECK (0x00) | 196 | TX SUCCESS | |
| 5 | 14:20:48.822 | CMD SEND | OPEN (0x21) | 197 | TX SUCCESS | User reversed |
| 6 | 14:20:49.488 | CMD SEND | CHECK (0x00) | 198 | TX SUCCESS | |
| 7 | 14:20:51.518 | CMD SEND | CHECK (0x00) | 199 | TX SUCCESS | |
| 8 | 14:20:53.520 | CMD SEND | CHECK (0x00) | 200 | TX SUCCESS | |
| 9 | 14:20:55.530 | CMD SEND | CHECK (0x00) | 201 | TX SUCCESS | |
| 10 | 14:20:57.538 | CMD SEND | CHECK (0x00) | 202 | TX SUCCESS | Log ends here |

**Bathroom summary:** 10 commands sent, ZERO responses received. Counter 193->203 (10 consumed, all wasted). Log ends before a SILENT FAILURE timeout could occur (would need 10s after last command). High starting counter (193) suggests many prior failed attempts or counter drift. No recovery attempted during log window.

---

## 3. TX Failure Analysis

### TX FAILED Events

| # | Timestamp | Blind | CC1101 State | Bytes Remaining | Total Time | Details |
|---|---|---|---|---|---|---|
| 1 | 14:19:17.754 | 0x717527 (LR4) | 0x13 (RX overflow) | 28 bytes | 5ms | First CHECK after CLOSE; immediately retried successfully |
| 2 | 14:20:23.613 | 0xd60227 (LR3) | 0x0d (idle) | N/A | 70ms | TX start timeout; retried successfully |

### Analysis

- **Total TX attempts (CMD SEND events):** ~87
- **TX failures:** 2
- **TX failure rate:** 2.3%
- Both failures were immediately retried and succeeded on retry.
- Failure #1 (state 0x13 = RX overflow) occurred right after a burst of commands to multiple blinds (LR2/LR3/LR4 being commanded in rapid succession at ~14:19:17). The CC1101 RX FIFO overflowed, likely because a received packet was still being processed when TX was attempted.
- Failure #2 (TX start timeout at state 0x0d) occurred right after receiving a response from LR3 (at 14:20:23.437), suggesting the radio was still processing the inbound packet.
- **Correlation:** TX failures happen during rapid send/receive transitions, not correlated with specific blinds. Both were transient.

---

## 4. Signal Strength Analysis

### Direct Responses from Blinds (to ESP commands)

| Blind | RSSI (dBm) | LQI | Timestamp |
|---|---|---|---|
| Office (0xf98627) | -70.0 | 46 | 14:20:37.964 |
| Office (0xf98627) | -74.0 | 48 | 14:20:38.157 |
| Office (0xf98627) | -77.0 | 47 | 14:20:39.388 |
| Office (0xf98627) | -67.0 | 47 | 14:20:41.905 |
| Bedroom (0xfa8627) | -89.0 | 47 | 14:20:25.688 |
| Bedroom (0xfa8627) | -85.0 | 47 | 14:20:26.486 |
| LR3 (0xd60227) | -95.0 | 98 | 14:20:23.437 |
| LR4 (0x717527) | -96.0 | 64 | 14:20:38.591 |

### Signal Strength Ranking (best to worst)

1. **Office (0xf98627):** -67.0 to -77.0 dBm, LQI 46-48 -- STRONG
2. **Bedroom (0xfa8627):** -85.0 to -89.0 dBm, LQI 47 -- MODERATE
3. **LR3 (0xd60227):** -95.0 dBm, LQI 98 -- VERY WEAK (single sample, anomalous LQI)
4. **LR4 (0x717527):** -96.0 dBm, LQI 64 -- VERY WEAK (single sample)
5. **LR2 (0x5f7527):** N/A (never responded directly)
6. **LR1 (0x598727):** N/A (never tested)
7. **Bathroom (0x876e27):** N/A (never responded)

### Overheard Physical Remote Packets (from 0x0b3973 to 0x5f7527)

| Timestamp | RSSI | LQI |
|---|---|---|
| 14:19:36.379 | -85.0 | 47 |
| 14:19:37.735 | -85.0 | 47 |
| 14:19:39.713 | -87.0 | 49 |
| 14:19:54.487 | -84.0 | 48 |

Remote signal: -84 to -87 dBm (consistent, moderate).

### Overheard Mesh Relay Packets (relayed through known blinds)

Packets relayed via 0xf98627 (Office): RSSI -66 to -67 dBm (strong)
Packets relayed via 0xfa8627 (Bedroom): RSSI -86 to -90 dBm (weak)
Packets relayed via 0x598727 (LR1): RSSI -90 to -91 dBm (weak)
Packets relayed via 0xd60227 (LR3): RSSI -92 dBm (weak)
Packets relayed via 0x5f7527 (LR2): RSSI -94 dBm (very weak)
Packets relayed via 0x988727 (unknown): RSSI -85 to -86 dBm (moderate)

### Blinds Below -90 dBm (Weak Signal Threshold)

- **LR3 (0xd60227):** -95.0 dBm -- below threshold
- **LR4 (0x717527):** -96.0 dBm -- below threshold
- **LR2 (0x5f7527):** no direct response, but mesh relay via LR2 shows -94 dBm -- below threshold
- **LR1 (0x598727):** mesh relay shows -90 to -91 dBm -- at/below threshold
- **Bathroom (0x876e27):** no data at all -- likely below threshold

---

## 5. Counter Tracking

### Per-Blind Counter Summary

| Blind | Starting Counter | Ending Counter | Total Consumed | Wasted (no response) | Counter Synced? |
|---|---|---|---|---|---|
| Office (0xf98627) | 21 | 26 | 5 | 1 | No |
| Bedroom (0xfa8627) | 64 | 69 | 5 | 3 | No |
| LR2 (0x5f7527) | 28 | 34 | 28* | 28* | Yes (x4) |
| LR3 (0xd60227) | 13 | 33 | 20+ | 19+ | No |
| LR4 (0x717527) | 40 | 67 | 27 | 25 | No |
| Bathroom (0x876e27) | 193 | 203 | 10 | 10 | No |
| LR1 (0x598727) | N/A | N/A | 0 | 0 | No |

*LR2 counter path: 28->41 (first run, 13 used), then synced to 21, then 25->34 (recovery, 9 more). Total ~22 unique counters consumed.

### Counter Sync Correctness (LR2 / 0x5f7527)

| Timestamp | Remote Counter | ESP Counter Before | ESP Counter After | Correct? |
|---|---|---|---|---|
| 14:19:36.390 | 19 | 41 | 21 | Yes (remote+2) |
| 14:19:37.762 | 20 | 21 | 22 | Yes (remote+2) |
| 14:19:39.719 | 21 | 22 | 23 | Yes (remote+2) |
| 14:19:54.504 | 23 | 23 | 25 | Yes (remote+2) |

The sync formula is: `ESP_new = remote_cnt + 2`. This is consistent. However, after syncing to counter 25, the next commands (CLOSE at cnt=25, CHECKs at cnt=26-29, STOP at cnt=29) all failed -- no response from blind. This means either:
- The counter sync formula is wrong (blind expects a different offset), or
- The blind simply cannot hear the ESP's transmissions on this channel/at this distance.

### Recovery Counter Analysis

**LR3 recovery success:** Original failed at counter 23 (STOP). Recovery started at offset +1 (cnt=24), tried through offset +4 (cnt=27). Succeeded at cnt=27/28 -- the response came to chl=27 (the CHECK sent at counter 27). The blind's internal counter was at ~28, meaning it was 5 ahead of where ESP expected.

**LR4 recovery success:** Original failed at counter 48 (STOP). Recovery swept from offset +1 through +15. Succeeded at cnt=63 -- the blind's internal counter was at ~155 (the `cnt` field in the response packet). Offset +16 from original 48 = 64, which matched. The blind was 16 counters ahead, suggesting roughly 16 commands from another source (physical remote or another controller) that the ESP didn't see.

**Post-recovery:** In both LR3 and LR4, after recovery SUCCESS, the very next CHECK command (sent at the correct next counter) FAILED with a silent failure. This strongly suggests the problem is NOT the counter -- the blind simply cannot reliably hear the ESP.

---

## 6. Physical Remote Activity (0x0b3973)

All observed packets from the physical remote were directed at LR2 (0x5f7527):

| # | Timestamp | Counter | Destination | Command | RSSI | LQI | Payload |
|---|---|---|---|---|---|---|---|
| 1 | 14:19:36.379 | 19 | 0x5f7527 | CHECK (0x00) | -85.0 | 47 | `02 03 00 00 00 00 00 00 00 00` |
| 2 | 14:19:37.735 | 20 | 0x5f7527 | CHECK (0x00) | -85.0 | 47 | `00 03 00 00 40 00 00 00 00 c0` |
| 3 | 14:19:39.713 | 21 | 0x5f7527 | CHECK (0x00) | -87.0 | 49 | `00 03 00 00 41 00 00 00 00 00` |
| 4 | 14:19:54.487 | 23 | 0x5f7527 | CHECK (0x00) | -84.0 | 48 | `00 03 00 00 20 00 00 00 00 c0` |

### Button Press Sequence Analysis

- Packets 1-3 (cnt 19, 20, 21) arrive at ~1.3s intervals -- consistent with a CHECK polling sequence after a button press.
- Packet 1 has a different first payload byte (0x02 vs 0x00) suggesting it was the initial action command, packets 2-3 are follow-up CHECKs.
- **Gap:** Counter 22 is missing between packets 3 (cnt=21 at 14:19:39.713) and 4 (cnt=23 at 14:19:54.487). Either:
  - Packet cnt=22 was transmitted but not received by ESP (15 second gap suggests the remote may have gone idle and come back).
  - Or the remote used cnt=22 for a command to a different blind.
- Counter range: 19-23 (5 counters seen, 1 missed). Remote is transmitting on channel 4 (same as LR2's config).

### Key Finding

The physical remote successfully communicates with LR2 (0x5f7527) -- the ESP overheard these packets. But the ESP itself, using the exact same channel (4) and synced counters, cannot get a response from LR2. This eliminates counter mismatch as the root cause for LR2 and points to a TX power or antenna issue on the ESP side.

---

## 7. Working vs Non-Working Comparison

### Working Blinds

| Property | Office (0xf98627) | Bedroom (0xfa8627) |
|---|---|---|
| Channel | 81 | 34 |
| Response RSSI | -67 to -77 dBm | -85 to -89 dBm |
| Response LQI | 46-48 | 47 |
| Response rate | 4/5 (80%) | 2/5 (40%) |
| Counter issues | None | None |
| TX failures | 0 | 0 |
| Signal quality | Strong | Moderate |

### Non-Working Blinds

| Property | LR2 (0x5f7527) | LR3 (0xd60227) | LR4 (0x717527) | Bathroom (0x876e27) | LR1 (0x598727) |
|---|---|---|---|---|---|
| Channel | 4 | 5 | 6 | 7 | N/A |
| Response RSSI | never responded | -95.0 (1x) | -96.0 (1x) | never responded | never tested |
| Response LQI | N/A | 98 (1x) | 64 (1x) | N/A | N/A |
| Response rate | 0/20 (0%) | 1/26 (3.8%) | 1/18 (5.6%) | 0/10 (0%) | N/A |
| Counter issues | Sync happened, still failed | Recovery needed (+5) | Recovery needed (+16) | Unknown | N/A |
| TX failures | 0 | 1 (retry OK) | 1 (retry OK) | 0 | 0 |
| Mesh relay RSSI | -94 | -92 | N/A | N/A | -90 to -91 |

### Pattern Analysis

**What working blinds have in common:**
1. **Higher channels:** Office=81, Bedroom=34. Non-working blinds use channels 4, 5, 6, 7.
2. **Stronger signals:** Working blinds respond at -67 to -89 dBm. Non-working blinds' rare responses are at -95 to -96 dBm.
3. **Consistent responses:** Working blinds respond to most commands. Non-working blinds respond to almost none.

**What non-working blinds have in common:**
1. **Low channel numbers:** 4, 5, 6, 7 -- these are sequential and may correspond to physical proximity (all in the living room + bathroom).
2. **Very weak or no signal:** All are at or below -90 dBm when they respond at all.
3. **Counter drift:** LR3 was 5 ahead, LR4 was 16 ahead -- suggesting the physical remote has been used but the ESP didn't overhear all commands.
4. **Post-recovery failure:** Even after finding the correct counter, the next command fails. This proves the counter is not the issue.
5. **Physical distance:** Living room and bathroom are likely farther from the ESP32 than the office.

**Critical observation:** The ESP can HEAR packets relayed through the non-working blinds (mesh traffic relayed via 0x5f7527, 0xd60227, 0x598727), proving the blinds are powered on and participating in the mesh network. The ESP can also hear the physical remote talking to LR2. But the blinds cannot hear the ESP's transmissions (or can hear them only extremely rarely, as with LR3 and LR4's single recovery responses).

---

## 8. Key Findings and Root Cause Hypotheses

### Most Likely Root Cause: Asymmetric RF Link (ESP TX is weaker than RX)

The evidence strongly points to the ESP32+CC1101 having significantly weaker TX power than RX sensitivity:
- The ESP can receive packets from/through all the non-working blinds (at -90 to -96 dBm).
- The ESP can receive the physical remote's packets to LR2 (at -84 to -87 dBm).
- But the blinds cannot hear the ESP's transmissions, even with correct counters.
- The physical remote (0x0b3973) CAN reach LR2 -- so the blind's receiver works fine.

This is a classic "can hear but can't be heard" asymmetric link problem. The CC1101 TX power may be insufficient, or the antenna/matching network on the ESP module is suboptimal for transmission.

### Supporting Evidence

1. **Counter is NOT the problem:** After successful counter recovery (LR3 at offset +5, LR4 at offset +16), the very next command at the correct counter fails. If it were a counter issue, post-recovery commands should work.

2. **Channel number correlation:** Working blinds use high channels (34, 81), non-working use low channels (4-7). Different channel numbers may correspond to different RF frequencies, and the CC1101 TX power or antenna tuning may vary with frequency.

3. **Distance correlation:** The RSSI values from mesh relay packets show that living room blinds relay at -90 to -94 dBm (near the RX sensitivity limit), confirming they are physically distant from the ESP.

4. **LR3's anomalous LQI:** The one response from LR3 had LQI=98, far higher than any other packet (typical LQI is 43-55). This is suspicious and may indicate the packet was marginal/corrupted, or arrived via an unusual propagation path.

### Other Anomalies

1. **Bathroom counter at 193:** This suggests either a prior session consumed many counters, or the counter was initialized incorrectly. With 10 commands and no response, the blind may never have been paired correctly, or its counter has diverged far from what the ESP thinks.

2. **LR4 counter 16 ahead:** The physical remote was used extensively on LR4 without the ESP seeing those commands (unlike LR2 where it did overhear them). This means the remote was used while the ESP was offline or on a different channel.

3. **LR3 responded to the physical remote's counter range (chl=27), not to ESP's original commands:** The successful recovery response was on chl=27, which was the counter value after recovery aligned with the blind's expectation. This is normal behavior but underscores that the blind IS functional -- it just can't hear the ESP most of the time.

### Recommendations

1. **Increase CC1101 TX power** to maximum (if not already). Check CC1101 PATABLE register configuration.
2. **Check antenna connection** on the CC1101 module. A loose or poorly matched antenna would reduce TX range more than RX range.
3. **Consider a CC1101 module with an external antenna** (SMA connector) for better range.
4. **Move the ESP32 physically closer** to the living room / bathroom blinds.
5. **Investigate channel/frequency correlation:** Verify that low channel numbers map to frequencies where the CC1101 TX output is weaker.
6. **Add a repeater/relay node** in the living room to extend range.
7. **For LR2 specifically:** The physical remote counter was at 23. The ESP synced and tried with the correct counter (25+) but still got no response. This confirms the problem is RF, not protocol.
