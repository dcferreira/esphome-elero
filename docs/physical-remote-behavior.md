# Physical Elero Remote Controller (0x0b3973) Observed Behavior

This document captures observed behavior of a physical Elero remote controller
(address `0x0b3973`) based on limited log analysis. It may not be complete.

## Packet Structure

- Each packet has a unique incrementing counter value that is never repeated.
- Approximately 1.3 seconds between consecutive packets.
- The remote waits for the blind to respond before sending the next packet.
- A single button press consumes 3-5 counter values.

## Button Press Sequences

### DOWN Sequence

1. **CHECK** polls sent to one or more blinds (status poll, command byte `0x00`).
2. **DOWN** command (`0x41`) sent to the target blind.
3. Post-command **CHECK** (`0x00`) to confirm the blind received the command.

### UP Sequence

1. **STOP** (`0x10`) sent first.
2. **UP** (`0x20`) sent to the target blind.
3. **UP variant** (`0x21`) follows immediately after.

## Command Bytes Observed

| Byte   | Name         | Description                              |
|--------|--------------|------------------------------------------|
| `0x00` | CHECK        | Status poll                              |
| `0x10` | STOP         | Stop movement                            |
| `0x20` | UP           | Move up                                  |
| `0x21` | UP variant   | Possibly UP + intermediate position flag |
| `0x40` | DOWN         | Move down                                |
| `0x41` | DOWN variant | Possibly DOWN + intermediate position flag |

## Key Differences from ESP Implementation

| Aspect | Physical Remote | ESP Implementation |
|--------|----------------|--------------------|
| Packets per counter | 1 | 2 (`ELERO_SEND_PACKETS=2`) |
| Inter-packet delay | ~1.3s (waits for blind response) | N/A |
| Pre-command status poll | Yes (CHECK before movement) | No |
| UP command | Sends STOP first, then `0x20`, then `0x21` | Uses only `0x21` |
| DOWN command | Uses `0x41` | Uses `0x40` |

Notable differences:

- The remote sends only 1 packet per counter value, whereas the ESP was
  configured to send 2 (`ELERO_SEND_PACKETS=2`).
- The remote waits approximately 1.3 seconds between packets for a response
  from the blind.
- The remote polls blind status (CHECK) before sending movement commands.
- The remote sends a STOP command before UP.
- The remote uses `0x20` for UP followed by `0x21`, while the ESP uses only
  `0x21`.

## Signal Characteristics

- Physical remote packets were received at RSSI -80 to -83 dBm.

## Open Questions

- Which steps in the button press sequences are strictly required by the
  protocol versus artifacts of the remote's UI behavior?
- Are `0x20` vs `0x21` (UP vs UP variant) and `0x40` vs `0x41` (DOWN vs DOWN
  variant) functionally different commands, or do the variants encode additional
  information such as an intermediate position flag?
- Is the pre-command CHECK poll required for reliable operation, or is it purely
  informational for the remote's display?
- Does sending STOP before UP matter for the blind's behavior, or is it a
  remote UI convention?
