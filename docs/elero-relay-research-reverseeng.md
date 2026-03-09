# Elero Protocol Relay/Mesh Research - Reverse Engineering Sources

Research compiled from open-source implementations, GitHub issues, and hobbyist reverse engineering projects.

Last updated: 2026-03-09

---

## Table of Contents

1. [Source Projects](#source-projects)
2. [Packet Structure](#packet-structure)
3. [The Hop Field](#the-hop-field)
4. [Backward (bwd) and Forward (fwd) Address Fields](#backward-bwd-and-forward-fwd-address-fields)
5. [The syst and typ Fields](#the-syst-and-typ-fields)
6. [Relay / Repeater / Mesh Behavior](#relay--repeater--mesh-behavior)
7. [Encryption and Encoding](#encryption-and-encoding)
8. [Known Issues with Relay/Mesh](#known-issues-with-relaymesh)
9. [Range Issues and Solutions](#range-issues-and-solutions)
10. [Packet Parameters Needed for Relay](#packet-parameters-needed-for-relay)
11. [Open Questions](#open-questions)

---

## Source Projects

### Primary Implementations

| Project | Author | URL | License | Description |
|---------|--------|-----|---------|-------------|
| esphome-elero | andyboeh | https://github.com/andyboeh/esphome-elero | - | ESPHome component for Elero blinds (C++). The original and most widely-used open-source implementation. |
| elero_protocol | QuadCorei8085 | https://github.com/QuadCorei8085/elero_protocol | MIT | C++ protocol implementation. Worked out the basic message structure and encryption. |
| eleropy | stanleypa | https://github.com/stanleypa/eleropy | GPLv3 | Python implementation with MQTT. Remote handling and status polling. |
| esphome-elero (fork) | pfriedrich84 | https://github.com/pfriedrich84/esphome-elero | - | Fork adding cover and light support, RF discovery scanning. |
| esphome-elero (fork) | manuschillerdev | https://github.com/manuschillerdev/esphome-elero | - | Active fork with ongoing development. |

### Key Issue Threads (Protocol Insights)

| Issue | URL | Key Finding |
|-------|-----|-------------|
| #5 "Blinds sometimes not moving" | https://github.com/andyboeh/esphome-elero/issues/5 | Hop field values 0x00 vs 0x0a in received packets; TX buffer overflow during simultaneous commands |
| #11 | https://github.com/andyboeh/esphome-elero/issues/11 | Blind transmit address differs from receive address; bwd/fwd change in relayed messages |
| #14 | https://github.com/andyboeh/esphome-elero/issues/14 | Varied hop values (0x0a, 0x1a, 0x2a, 0x35) confirming multi-hop relay; different bwd/fwd pairs prove relay routing |
| #15 | https://github.com/andyboeh/esphome-elero/issues/15 | Hop 0x00 outbound becomes 0x0a in responses; relay path visible in bwd/fwd |
| #18 | https://github.com/andyboeh/esphome-elero/issues/18 | Maintainer confirms hop=0x00 means "not relayed, must be near blind" |
| #23 | https://github.com/andyboeh/esphome-elero/issues/23 | Multiple relay hops (0x15, 0x25, 0x05) observed; varying typ2 values (0x10-0x13) |
| #24 | https://github.com/andyboeh/esphome-elero/issues/24 | Maintainer confirms: hop=0x00 disables relay; removing hop=0x00 constraint enables mesh relay; src/bwd/fwd differ when relayed |
| #29 | https://github.com/andyboeh/esphome-elero/issues/29 | User tried adjusting hop parameter; len=27 vs len=29 packet types |

---

## Packet Structure

The Elero protocol uses variable-length packets on 868 MHz (GFSK modulation, 76.8 kBaud, 35 kHz deviation, NRZ encoding).

### Complete Packet Layout

```
Offset  Field        Size    Description
------  -----        ----    -----------
0       msg_len      1       Packet length: 0x1D (29) for control commands (typ >= 0x60)
                              0x1B (27) for internal/programming messages (typ < 0x60)
1       pck_cnt      1       Rolling counter, incremented per transmission
2       typ          1       Packet type / pck_inf[0] (see typ field section)
3       typ2         1       Packet info 2 / pck_inf[1] (typically 0x10)
4       hop          1       Hop/relay information byte (see hop section)
5       syst         1       System address (always 0x01 in known implementations)
6       chl          1       Channel number
7-9     src          3       Source address (24-bit, big-endian)
10-12   bwd          3       Backward address (24-bit, relay path backward)
13-15   fwd          3       Forward address (24-bit, relay path forward)
16      dest_count   1       Number of destinations (typically 0x01)
17-19   dst          3       Destination address (3 bytes when typ >= 0x60)
                     or 1    Destination channel (1 byte when typ < 0x60)
20-21   payload_hdr  2       Payload header (payload[0], payload[1])
22-29   payload_enc  8       Encoded/encrypted payload data
```

**Appended by CC1101 hardware (not part of transmitted packet):**
```
len+1   rssi         1       Received Signal Strength Indicator
len+2   lqi_crc      1       Bits 0-6: Link Quality Indicator; Bit 7: CRC OK flag
```

### Packet Length Rules

- `len = 0x1D (29)`: Standard control commands (typ >= 0x60, e.g., 0x6A). Uses 3-byte destination addresses.
- `len = 0x1B (27)`: Internal/programming messages (typ < 0x60, e.g., 0x44). Uses 1-byte destination (channel only).

Source: eleropy `generate_msg()` method, elero_protocol `main.cpp`, esphome-elero `send_command()` and `interpret_msg()`.

---

## The Hop Field

### What is Known

The hop field at byte offset 4 controls relay/repeater participation in the Elero mesh network. This is one of the most critical fields for reliable communication.

### Observed Hop Values

| Value | Context | Meaning |
|-------|---------|---------|
| `0x00` | Sent by QuadCorei8085 implementation for programming commands | No relay / direct communication only |
| `0x05` | Used by eleropy for normal operations | Standard relay-enabled operation |
| `0x0a` | Default in esphome-elero; seen in many received packets | Common relay-enabled value |
| `0x0a`, `0x1a`, `0x2a`, `0x3a` | Received in issue #14 logs | Possibly encodes hop count or relay depth |
| `0x05`, `0x15`, `0x25`, `0x35` | Received in issue #23 logs | Another series suggesting incrementing hop counts |

### Maintainer Statements

From andyboeh (esphome-elero maintainer), issue #18:
> "Since `hop` is set to `0x00`, you need to be very near the receiving blind as the commands are not relayed."

From issue #24:
> Setting hop to 0x00 means "no hop" -- the transmitter must be in close proximity. Removing the `hop: 0x00` constraint allows the system to use mesh relay capabilities.

### Interpretation

The hop byte appears to use a **composite encoding**:
- The **low nibble** (bits 0-3) may indicate the maximum allowed hop count or relay permission level.
- The **high nibble** (bits 4-7) may indicate the current hop count or relay depth.

Evidence: Values like 0x05, 0x15, 0x25, 0x35 share the same low nibble (5) but have incrementing high nibbles (0, 1, 2, 3), suggesting the high nibble is incremented by each relay node.

Similarly: 0x0a, 0x1a, 0x2a, 0x3a share low nibble 0xa with incrementing high nibbles.

**Setting hop to 0x00 disables relaying entirely.** This is confirmed by the maintainer and by multiple user reports where hop=0x00 required close proximity to the blind.

---

## Backward (bwd) and Forward (fwd) Address Fields

### What is Known

The protocol uses three 24-bit address fields for routing:

1. **src** (bytes 7-9): The original source/sender address
2. **bwd** (bytes 10-12): Backward relay address -- the address of the previous hop
3. **fwd** (bytes 13-15): Forward relay address -- the address of the next hop or relay target

### Direct Communication (No Relay)

When a command is sent directly (no intermediate relays), all three fields are set to the same address:

```
src = remote_addr
bwd = remote_addr
fwd = remote_addr
```

This is the pattern used by all current open-source implementations when transmitting commands. From esphome-elero `send_command()`:
```cpp
msg_tx_[7-9]   = remote_addr;  // source address
msg_tx_[10-12] = remote_addr;  // backward address (same as source)
msg_tx_[13-15] = remote_addr;  // forward address (same as source)
```

From the README: "If there are multiple lines, look for the line where `src`, `bwd` and `fwd` all have the same values."

### Relayed Communication

When messages are relayed through intermediate blinds, the three address fields DIFFER:

From issue #14 log examples:
```
bwd=0xeb813a, fwd=0xc7813a    (different addresses = relayed)
bwd=0xc7813a, fwd=0x419c40    (relay chain continues)
```

From issue #24, the maintainer confirmed:
> "src, bwd and fwd address are not equal" when messages are relayed.

### Interpretation

The bwd/fwd fields appear to implement **source routing** or **breadcrumb routing**:

- **bwd**: Updated by each relay node to its own address, creating a backward path trace
- **fwd**: Updated by each relay node to indicate the next-hop destination
- **src**: Remains the original sender's address throughout the relay chain

This allows:
1. Response messages to follow the reverse path back
2. Each relay node to know where to forward the message
3. The destination blind to know the original sender

### Key Finding from Issue #11

> "The address used by the remote to send signals to the blind differs from the address used by the blind to report its state back."

Blinds have separate TX and RX addresses. Status responses (typ=0xCA/0xC9) use the blind's own address as `src`, while commands are addressed to the blind's receive address as `dst`. This asymmetric addressing is critical for correct configuration.

---

## The syst and typ Fields

### syst (System Address) - Byte 5

- Always `0x01` in all known implementations and captured traffic
- Purpose not fully understood; may identify the Elero system/protocol version
- No known variation has been documented

### typ (Packet Type) - Byte 2

The typ field (also called `pck_inf[0]`) determines the packet type and affects packet structure:

| Value | Direction | Description |
|-------|-----------|-------------|
| `0x6A` | Remote -> Blind | Standard control command (up/down/stop/check) |
| `0xCA` | Blind -> Remote | Status response (state report) |
| `0xC9` | Blind -> Remote | Status response (alternate type) |
| `0x44` | Internal | Internal message (len=27, 1-byte destination) |
| `0x70` | Remote -> Blind | Programming/pairing command |
| `0xF8` | Remote -> Blind | Programming/pairing command |
| `0x78` | Remote -> Blind | Programming/pairing command |

**Structural impact of typ:**
- When `typ >= 0x60`: Packet uses 3-byte destination addresses (len=29)
- When `typ < 0x60`: Packet uses 1-byte destination/channel (len=27)

### typ2 (Packet Info 2) - Byte 3

- Typically `0x10` for transmitted commands
- Received packets show variation: `0x10`, `0x11`, `0x12`, `0x13`
- The varying values in received packets (issue #23) may encode relay depth or retransmission count
- Not well understood; may be related to bidirectional protocol handshaking

---

## Relay / Repeater / Mesh Behavior

### How Blinds Act as Repeaters

Elero blinds with wireless receivers can function as **signal repeaters** for other blinds in the same installation. This creates an ad-hoc mesh network.

**Key evidence:**

1. **Hop field values change with relay depth** (issues #14, #23): Received packets show incrementing high nibbles (0x0a -> 0x1a -> 0x2a) indicating each relay node increments the hop count.

2. **bwd/fwd addresses change at each hop** (issue #14): Each relay node updates the backward and forward address fields to create a traceable route.

3. **Maintainer confirmation** (issue #24): Enabling hop allows "blinds to relay commands when direct range was inadequate."

### Relay Mechanism (Inferred)

Based on observed behavior, the relay mechanism works as follows:

1. **Transmitter sends command** with hop field set to a non-zero value (e.g., 0x0a)
   - src = bwd = fwd = transmitter address (direct send)

2. **Nearby blind receives the command** but it is not the destination
   - The blind checks if hop field permits further relaying
   - If yes, it retransmits the message with:
     - hop high nibble incremented (e.g., 0x0a -> 0x1a)
     - bwd updated to its own address
     - fwd updated to the next target or left for broadcast
     - src remains the original transmitter address

3. **Process repeats** until the destination blind receives the message or maximum hops are reached

4. **Response travels back** using the bwd breadcrumb trail

### What Makes Relay Work

For relay/mesh to function, the following conditions must be met:

- **hop field must be non-zero** (0x00 disables relay entirely)
- The transmitter must be **paired/programmed** into the blind's known device list
- Intermediate blinds must be **powered on** and within RF range of each other
- The Elero protocol appears to handle relay transparently at the firmware level -- no explicit relay configuration is needed beyond pairing

### Relay is Transparent to Open-Source Implementations

None of the open-source implementations (esphome-elero, eleropy, elero_protocol) implement relay/repeater logic. They only send direct commands. However, the Elero blind firmware handles relaying automatically:

- Commands sent with hop != 0x00 are eligible for relay
- Blinds that receive non-destined commands may re-broadcast them
- The open-source transmitter just needs to set the hop field correctly

---

## Encryption and Encoding

The payload (bytes 20-29) undergoes multi-layer obfuscation. This is not cryptographic security.

### Rolling Code

Each command includes a counter-derived code:
```
code = (0x00 - (counter * 0x708F)) & 0xFFFF
```
This is placed at bytes 22-23 before encoding.

### Encoding Steps (Transmit)

1. **Parity calculation**: 4 parity bits computed from pairs of bytes
2. **Nibble offset addition**: Starting value 0xFE, decrementing by 0x22 per byte; added to each nibble independently
3. **XOR masking**: Bytes 2-7 XORed alternately with bytes 0 and 1
4. **Nibble substitution**: Each nibble transformed via 16-entry lookup table (`flash_table_encode`)

### Decoding Steps (Receive)

1. **Nibble substitution** (reverse table: `flash_table_decode`)
2. **Nibble offset subtraction** (0xFE start, -0x22 per step) for bytes 0-1
3. **XOR unmasking** using decoded bytes 0 and 1
4. **Nibble offset subtraction** (0xBA start) for bytes 2-7

### Command Byte (Decoded Payload Byte 4)

| Value | Command |
|-------|---------|
| `0x00` | Status check (CHECK) |
| `0x10` | Stop (STOP) |
| `0x20` | Open/Up (UP) |
| `0x24` | Tilt (TILT) |
| `0x40` | Close/Down (DOWN) |
| `0x44` | Intermediate position (INT) |

### Security Note

From QuadCorei8085: The encryption is "byte-swapping + xoring + magic addition" and is "nothing fancy." The counter provides basic replay protection but is not cryptographically secure. The rolling counter can be predicted.

---

## Known Issues with Relay/Mesh

### 1. hop=0x00 Silently Disables Relay

Setting `hop: 0x00` in the ESPHome configuration completely disables relay functionality, requiring the ESP32+CC1101 module to be within direct RF range of every blind. This was the root cause of multiple reported issues (#18, #24).

**Impact**: Blinds that are far from the ESP32 module but reachable via other blinds will not respond.

**Fix**: Use a non-zero hop value. The default `0x0a` works in most installations. The eleropy project uses `0x05`.

### 2. Simultaneous Commands Cause TX Buffer Overflow

From issue #5: Sending commands to multiple blinds simultaneously can cause:
- "Error transferring, 28 bytes left in buffer"
- "Timed out waiting for TX: 0x0d"
- "Component elero.cover took a long time for an operation (60 ms)"

**Impact**: Commands are silently lost.

**Current mitigation**: Serial command queuing with delays between transmissions (50ms in esphome-elero).

### 3. Asymmetric Blind Addresses

From issue #11: A blind's transmit address (used in status messages) differs from its receive address (used to accept commands). Users must capture traffic from the physical remote to determine the correct receive address.

### 4. Status Check Interferes with Movement

From issue #15: Sending a CHECK (0x00) command while a blind is moving causes the blind to interpret it as a stop signal. The esphome-elero component had to add logic to avoid polling during movement.

### 5. Relayed Status Messages Have Different Addressing

When status responses are relayed through intermediate blinds, the src/bwd/fwd fields may not match the expected pattern, potentially confusing simple implementations that only check the src field.

### 6. Counter Desynchronization

The rolling counter must stay synchronized between the transmitter and blind. If a physical remote is used, the counter advances without the ESP32 knowing. This project implements counter recovery with a sweep of +/-5 values.

---

## Range Issues and Solutions

### Typical Range Problems

1. **CC1101 module quality varies**: Different suppliers produce modules with significantly different RF performance. Some modules are unreliable.

2. **Antenna matters**: The eleropy README states that a standard 82mm dipole antenna improves signal by **10-20 dB** compared to coil antennas often shipped with cheap CC1101 modules.

3. **Frequency offset**: CC1101 modules often have slight frequency deviations. The esphome-elero component allows tuning via `freq0`, `freq1`, `freq2` registers. Users should capture signals from their physical remote using an SDR (e.g., SDRangel) to determine the exact frequency.

4. **hop=0x00 disables relay**: The single most common cause of range issues in esphome-elero installations. See issue #18, #24.

### Solutions

| Solution | Impact | Source |
|----------|--------|--------|
| Set hop to 0x0a (default) | Enables blind-to-blind relay | Issues #18, #24 |
| Use 82mm dipole antenna | +10-20 dB signal improvement | eleropy README |
| Tune CC1101 frequency registers | Correct for module frequency offset | esphome-elero README |
| Position ESP32 centrally | Minimize maximum distance to any blind | General advice |
| Increase TX power (PATABLE) | Maximum CC1101 output power | esphome-elero uses 0xC0 (max) |
| Multiple ESP32 modules | Independent controllers for distant groups | Last resort |

### RF Configuration Details

- **Frequency**: 868.xx MHz (exact frequency varies; use SDR to determine)
- **Modulation**: GFSK
- **Baud rate**: 76.8 kBaud
- **Deviation**: 35 kHz
- **RX Bandwidth**: 232 kHz
- **TX Power**: 0xC0 in PATABLE (maximum, ~10 dBm)
- **Sync word**: 0xD391

Source: Issue #30 (modulation details from andyboeh).

---

## Packet Parameters Needed for Relay

Based on all available evidence, these are the critical parameters for relay-capable communication:

### Must Be Correct

| Parameter | Value | Notes |
|-----------|-------|-------|
| `hop` | Non-zero (0x0a recommended, 0x05 also works) | 0x00 disables relay entirely |
| `src` | Remote address | Must match a paired remote |
| `bwd` | Same as src for direct TX | Updated by relay nodes |
| `fwd` | Same as src for direct TX | Updated by relay nodes |
| `dst` | Blind's receive address | May differ from blind's TX address |
| `syst` | 0x01 | Always observed as 0x01 |
| `pck_inf[0]` / typ | 0x6A for control commands | Determines packet structure |
| `pck_inf[1]` / typ2 | 0x10 | Standard value |
| `counter` | Must be in sync with blind | Rolling counter tracked per remote-blind pair |
| `channel` | Correct channel for the blind | Paired during programming |

### Important But Less Critical

| Parameter | Value | Notes |
|-----------|-------|-------|
| `payload[0-1]` | 0x00, 0x03 (common) | Payload header; may vary by installation |
| `dest_count` | 0x01 | Number of destinations |
| `msg_len` | 0x1D (29) | For control commands |

---

## Open Questions

The following aspects of the Elero relay/mesh protocol remain poorly understood:

1. **Exact hop field encoding**: Is it truly high-nibble=count, low-nibble=max? What is the maximum hop count? Does a blind refuse to relay if the count exceeds some threshold?

2. **How do blinds decide to relay?** Is it based on:
   - Knowing the destination address from its pairing table?
   - Simply re-broadcasting anything with hop != 0x00?
   - Some RSSI-based decision?

3. **What does typ2 variation mean?** Values 0x10, 0x11, 0x12, 0x13 have been observed in received packets. Are these related to relay retransmission counts?

4. **Can we implement relay in software?** Could an ESP32 node act as a relay/repeater for Elero commands to extend range without additional hardware?

5. **What is the syst field for?** Always 0x01 -- is this a system/network identifier? Could there be multiple independent Elero networks using different syst values?

6. **Pairing table and relay eligibility**: Does a blind only relay commands for destinations it knows about, or does it relay blindly?

7. **Response routing**: How does the response (typ=0xCA/0xC9) find its way back through the relay chain? Does it use the bwd field as a breadcrumb trail, or does the blind simply broadcast the response?

8. **Timeout and retry at relay level**: Do relay nodes implement their own retry logic, or is retransmission purely the responsibility of the original sender?

---

## Sources

- https://github.com/andyboeh/esphome-elero -- Primary ESPHome implementation (andyboeh)
- https://github.com/QuadCorei8085/elero_protocol -- Original protocol reverse engineering (QuadCorei8085)
- https://github.com/stanleypa/eleropy -- Python implementation with MQTT (stanleypa)
- https://github.com/pfriedrich84/esphome-elero -- Fork with cover/light support and RF scanning
- https://github.com/manuschillerdev/esphome-elero -- Active fork
- https://github.com/andyboeh/esphome-elero/issues/5 -- Simultaneous command failures, hop values in logs
- https://github.com/andyboeh/esphome-elero/issues/11 -- Asymmetric blind addresses, bwd/fwd in relayed messages
- https://github.com/andyboeh/esphome-elero/issues/14 -- Multi-hop evidence (0x0a/0x1a/0x2a/0x3a), frequency config
- https://github.com/andyboeh/esphome-elero/issues/15 -- Hop changes in responses, CHECK interferes with movement
- https://github.com/andyboeh/esphome-elero/issues/18 -- Maintainer confirms hop=0x00 disables relay
- https://github.com/andyboeh/esphome-elero/issues/23 -- Multiple hop values (0x05/0x15/0x25), varying typ2
- https://github.com/andyboeh/esphome-elero/issues/24 -- Definitive confirmation: hop=0x00 disables relay; bwd/fwd differ when relayed
- https://github.com/andyboeh/esphome-elero/issues/29 -- Hop parameter adjustment attempts
- https://github.com/andyboeh/esphome-elero/issues/30 -- GFSK modulation parameters
