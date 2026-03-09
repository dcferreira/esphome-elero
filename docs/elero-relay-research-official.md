# Elero Signal Relay/Mesh Research - Official Documentation Sources

Research compiled from official Elero documentation, product manuals, and manufacturer website content.

Last updated: 2026-03-09

---

## Table of Contents

1. [Summary of Findings](#summary-of-findings)
2. [TempoTel 2 Official Manual](#tempottel-2-official-manual)
3. [ProLine 2 Bidirectional Radio System](#proline-2-bidirectional-radio-system)
4. [Technical Specifications](#technical-specifications)
5. [Relay Behavior - Official Description](#relay-behavior---official-description)
6. [The Hop Field and Protocol Implications](#the-hop-field-and-protocol-implications)
7. [Rated RF Range](#rated-rf-range)
8. [Repeater Products](#repeater-products)
9. [Multi-Hop Routing](#multi-hop-routing)
10. [Configuration Requirements](#configuration-requirements)
11. [Troubleshooting Clues from Manual](#troubleshooting-clues-from-manual)
12. [Sources](#sources)

---

## Summary of Findings

The TempoTel 2 official manual (page 10, "Funktionserklarung") explicitly describes relay/mesh behavior as a core feature of the bidirectional radio system. Key findings:

- **Signal relaying is automatic and built into the bidirectional protocol.** No user configuration is required.
- **Every bidirectional receiver acts as a relay node.** If the direct path to the target is blocked, the signal is forwarded through other bidirectional participants until it reaches the destination.
- **Relaying requires ALL participants to be bidirectional.** If any receiver in the chain is unidirectional, the relay chain breaks.
- **There are no separate repeater/relay products.** Elero does not sell dedicated repeaters -- every bidirectional receiver IS a repeater.
- **ProLine 2 is the name of the bidirectional radio technology**, not a separate product. It is built into all current Elero "-868" wireless products.
- The manual does NOT specify a rated RF range in meters/feet. It states that range is determined by legal limits and building conditions.
- Transmit power is specified as <= 500 mW (approximately +27 dBm), which is the maximum allowed under European 868 MHz regulations.

---

## TempoTel 2 Official Manual

### Source Document

- **Title**: TempoTel 2 - Bedienungsanleitung (Original Operating Instructions)
- **Document Number**: 18 202.0002_DE_0922
- **Article Numbers**: 28 265.0002, 28 266.0002, 28 267.0002, 28 265.0902, 28 266.0902, 28 267.0902
- **Publisher**: elero GmbH, Antriebstechnik, Maybachstr. 30, 73278 Schlierbach, Deutschland
- **Date**: Published 2019-02-26 on elero download portal
- **Downloaded from**: https://www.elero.de/de/downloads-service/downloads (search "TempoTel")
- **Format**: PDF, 1 MB, 39 pages, German language

### Important Notice (Page 2)

> **"Sender ist fur bidirektionalen Funkbetrieb voreingestellt!"**
> (Transmitter is factory preset for bidirectional radio operation!)

This confirms the TempoTel 2 ships in bidirectional mode by default. A DIP switch on the back of the device can restrict it to unidirectional-only mode (page 34).

---

## ProLine 2 Bidirectional Radio System

### Official Description from Elero Product Pages

From the elero.de product catalog page for "Steuerungen fur Rohrantriebe" (Controls for Tubular Motors):

> **German original:**
> "elero prasentiert seine gesamte Produktpalette standardmassig mit dem bidirektionalen Funksystem ProLine 2, das Ruckmeldung und Sicherheit uber alle versendeten Signale gibt. Eine echte Routing-Funktion sorgt dafur, dass die Funksignale uber einen anderen Funkempfanger weitergeleitet werden, falls der direkte Weg zum Antrieb, z. B. durch reflektierende Wande, nicht moglich ist."

> **English translation:**
> "elero presents its entire product range as standard with the bidirectional radio system ProLine 2, which provides feedback and security for all transmitted signals. A genuine routing function ensures that radio signals are relayed via another radio receiver if the direct path to the drive -- for example due to reflective walls -- is not possible."

### Key Takeaways

1. **ProLine 2 is standard across the entire product range** -- it is not an optional add-on or separate product.
2. **"Echte Routing-Funktion"** (genuine routing function) -- Elero explicitly calls this routing, not just signal boosting.
3. **Automatic relay**: Signals are relayed through other receivers when direct communication fails.
4. **Building-aware**: The example given (reflective walls) shows this is designed for real-world indoor environments where RF propagation is challenging.

Source: https://www.elero.de/de/produkte/steuerungen-fuer-rohrantriebe/

---

## Technical Specifications

From the TempoTel 2 manual, page 7 ("Technische Daten"):

| Parameter | Value |
|-----------|-------|
| Device name | TempoTel 2 |
| Operating voltage | 3 V DC |
| Battery type | 2 x LR06 (AA, Mignon) |
| Protection class | IP20 |
| Temperature range | 0 ... 55 degrees C |
| Radio frequency | 868 MHz Band |
| Transmit power [mW] | <= 500 |
| Dimensions [mm] (remote) | L 150 x B 51 x H 26 |
| Weight [g] (incl. batteries) | 140 |

For USA, Canada, Australia, and some South American countries:

| Parameter | Value |
|-----------|-------|
| Radio frequency | 915 MHz Band |

### Notable Specifications

- **Transmit power <= 500 mW**: This is approximately +27 dBm. This is the European regulatory maximum for 868 MHz SRD (Short Range Device) applications. The TempoTel 2 transmits at the maximum allowed power. By comparison, a typical CC1101 module used with ESP32 transmits at approximately +10 dBm (10 mW) -- roughly **50 times less power** than the official remote.
- **No range specification**: The manual deliberately omits a range figure. Page 5 states: "Die Reichweite des Funksignals ist durch den Gesetzgeber und die baulichen Bedingungen begrenzt" (The range of the radio signal is limited by legislation and building conditions). This is likely because the effective range depends entirely on the mesh relay chain -- with enough intermediate receivers, there is no fixed maximum range.

---

## Relay Behavior - Official Description

### From TempoTel 2 Manual, Page 10: "Funktionserklarung" (Functional Description)

This is the most authoritative source on relay behavior. The manual describes two distinct operating modes:

#### Bidirectional Radio System (Bidirektionales Funksystem)

> **German original:**
> "Bidirektionales Funksystem bedeutet die Ubertragung von Funksignalen an Funkempfanger und die Ruckmeldungsmoglichkeit der Funkempfanger an den Sender. Das Funksignal kann direkt an den Zielempfanger geschickt werden. Ist das nicht moglich, wird das Funksignal solange uber andere bidirektionale Teilnehmer weitergegeben, bis das Signal den Zielempfanger erreicht. Der Zielempfanger fuhrt den Befehl aus und schickt eine Bestatigung an den Sender zuruck.
> Bidirektionaler Funkbetrieb ist nur moglich, wenn alle Teilnehmer bidirektional sind. Sonst ist das System nur unidirektional."

> **English translation:**
> "Bidirectional radio system means the transmission of radio signals to radio receivers and the ability for radio receivers to send feedback to the transmitter. The radio signal can be sent directly to the target receiver. If that is not possible, the radio signal is forwarded through other bidirectional participants until the signal reaches the target receiver. The target receiver executes the command and sends a confirmation back to the transmitter.
> Bidirectional radio operation is only possible when ALL participants are bidirectional. Otherwise the system operates only in unidirectional mode."

#### Unidirectional Radio System (Unidirektionales Funksystem)

> **German original:**
> "Unidirektionales Funksystem bedeutet die Ubertragung von Funksignalen an Funkempfanger. Jedoch konnen die Funkempfanger, im Gegensatz zum bidirektionalen Funksystem, keine Ruckmeldung an den Sender zuruckschicken. Auch nicht moglich ist die Weitergabe von Funksignalen von Funkempfanger zu Funkempfanger."

> **English translation:**
> "Unidirectional radio system means the transmission of radio signals to radio receivers. However, unlike the bidirectional radio system, the radio receivers cannot send feedback to the transmitter. Also not possible is the forwarding of radio signals from receiver to receiver."

### Analysis of Official Relay Description

1. **Relay is automatic**: The manual says the signal "is forwarded" (wird weitergegeben) -- passive voice, implying the system handles this automatically without user intervention.

2. **Multi-hop is explicitly described**: "forwarded through other bidirectional participants **until** the signal reaches the target" -- the word "until" (bis) implies an iterative process with potentially multiple hops.

3. **ALL participants must be bidirectional**: This is a critical constraint. If even one receiver in the mesh is unidirectional, the relay chain cannot pass through it. The entire system degrades to unidirectional mode if mixed.

4. **Confirmation/response is part of the protocol**: The target receiver "sends a confirmation back to the transmitter" -- this confirms the response also travels back through the relay chain.

5. **Unidirectional mode explicitly lacks relay**: The manual explicitly states that unidirectional receivers CANNOT forward signals to each other. This confirms relay is exclusively a bidirectional feature.

---

## The Hop Field and Protocol Implications

The official manual does not mention the "hop" field by name -- it is a protocol-level implementation detail not exposed to end users. However, the manual's description of relay behavior aligns perfectly with what has been reverse-engineered from the protocol:

| Official Manual Description | Protocol-Level Implementation |
|---|---|
| "Signal is forwarded through other participants" | The `hop` byte at packet offset 4 controls relay eligibility. Non-zero values enable relay. |
| "Until the signal reaches the target" | The high nibble of the hop byte appears to increment at each relay node (0x0a -> 0x1a -> 0x2a), providing a TTL/hop-count mechanism. |
| "All participants must be bidirectional" | Bidirectional receivers have firmware that re-broadcasts received packets with updated bwd/fwd addresses. Unidirectional receivers lack this firmware. |
| "Confirmation sent back to transmitter" | Response packets (typ=0xCA/0xC9) use the bwd address field as a breadcrumb trail to route back through the relay chain. |

### What the Manual Implies About hop=0x00

The esphome-elero maintainer (andyboeh) confirmed in issue #18 and #24 that setting `hop: 0x00` disables relay entirely. This is consistent with the manual's description: if the protocol field that enables relay is set to zero, the system effectively operates in "direct only" mode, similar to unidirectional behavior but with feedback capability.

The default value of `0x0a` in esphome-elero enables relay and appears to work correctly with the automatic relay mechanism described in the official documentation.

---

## Rated RF Range

### Official Position: No Fixed Range Specification

The TempoTel 2 manual (page 5, "Sicherheitshinweise Funkbetrieb") states:

> "Die Reichweite des Funksignals ist durch den Gesetzgeber und die baulichen Bedingungen begrenzt."
> (The range of the radio signal is limited by legislation and building conditions.)

No specific range in meters or feet is given anywhere in the 39-page manual. This is intentional:

1. **With relay**: Effective range is theoretically unlimited, as signals hop through intermediate receivers.
2. **Without relay**: Range depends on building materials, interference, and regulatory power limits.
3. **Regulatory limits**: 868 MHz SRD regulations in Europe limit transmit power (which the TempoTel 2 uses at maximum: 500 mW / +27 dBm).

### Practical Range Observations

From the esphome-elero project's own log analysis:
- **Office blind (direct, strong signal)**: RSSI -67 to -77 dBm, 80% command success rate
- **Bedroom blind (direct, moderate signal)**: RSSI -85 to -89 dBm, 40% command success rate
- **Living room blinds (weak/no signal)**: RSSI -90 to -96 dBm, 0-5% command success rate

The CC1101 module used by the ESP32 transmits at approximately +10 dBm (10 mW), which is **17 dB weaker** than the official TempoTel 2 remote (500 mW / +27 dBm). This 50x power difference is the primary reason for the asymmetric link problem documented in the log analysis: the ESP can hear blind responses (RX is adequate) but blinds cannot hear the ESP's commands (TX is too weak for distant blinds).

### Troubleshooting Table (Page 36)

The manual's troubleshooting table lists this relevant entry:

| Problem | Cause | Solution |
|---------|-------|----------|
| Drive does not run, status LED red or orange blinking, unidirectional: status LED green | 1. Receiver outside radio range 2. Receiver out of service or defective 3. Receiver not yet paired | 1. Reduce distance to receiver 2. Turn on receiver or replace 3. Pair receiver |

This confirms that "outside radio range" is a recognized failure mode, and the solution is to "reduce distance" -- or, implicitly, to ensure intermediate bidirectional receivers are present to form a relay chain.

---

## Repeater Products

### Finding: No Dedicated Repeater Products Exist

After reviewing the complete Elero product catalog (45 products in the "Steuerungen fur Rohrantriebe" category across 5 pages), **no dedicated repeater, signal booster, or range extender product was found**.

Products reviewed include:
- Hand-held transmitters: TempoTel 2, VarioTel 2, LumeroTel 2, MonoTel 2, VarioCom, etc.
- Wall-mounted transmitters: MonoSon W-868, VarioSon-868 Slide, etc.
- Wireless receivers: VarioTec-868, Revio-868, Combio-868 RM, etc.
- Sensors: Protero-868, Sensero-868, Aero-868, Lumero-868, Lumo-868
- Specialty: ExitSafe, QuinTec-868, UniTec-868, Invio-868

**Every bidirectional receiver IS a repeater.** This is the design philosophy of ProLine 2: the mesh network is formed automatically by all installed bidirectional receivers. There is no need for a separate repeater product because:

1. Each blind motor's wireless receiver participates in the mesh.
2. Each wall-mounted receiver or sensor can relay signals.
3. The system is self-healing -- adding any new bidirectional device to the installation extends the mesh.

### Implication for ESP32 Installations

Since there is no official repeater product, the recommended approach for extending range is:
- Ensure all receivers in the installation are bidirectional (ProLine 2 compatible, "-868" suffix products)
- Position the ESP32+CC1101 module where it can reach at least ONE bidirectional receiver, which will then relay to others
- Use the maximum hop value to enable relay (default `0x0a` in esphome-elero)

---

## Multi-Hop Routing

### Official Confirmation

The TempoTel 2 manual explicitly describes multi-hop routing:

> "the radio signal is forwarded through other bidirectional participants **until** the signal reaches the target receiver"

The word "until" (German: "bis") implies:
1. The signal may pass through **multiple** intermediate nodes
2. The process is iterative -- each node that is not the target forwards the signal onward
3. There is a termination condition (reaching the target)

### Protocol-Level Evidence

From reverse engineering (documented in the companion file `elero-relay-research-reverseeng.md`):
- Hop values 0x0a, 0x1a, 0x2a, 0x3a have been observed, suggesting up to 3+ relay hops
- The high nibble increment pattern (0, 1, 2, 3) is consistent with a TTL/hop-count mechanism
- The maximum hop count is unknown but is likely encoded in the low nibble of the hop byte

### Response Routing

The manual confirms that responses travel back through the relay chain:

> "The target receiver executes the command and sends a confirmation back to the transmitter."

The protocol uses the `bwd` (backward) address field to trace the return path. Each relay node updates this field with its own address, creating a breadcrumb trail for the response.

---

## Configuration Requirements

### For Relay to Work (From Official Manual)

1. **All participants must be bidirectional**: The manual is explicit -- "Bidirektionaler Funkbetrieb ist nur moglich, wenn alle Teilnehmer bidirektional sind." Mixing bidirectional and unidirectional receivers breaks the mesh.

2. **TempoTel 2 ships in bidirectional mode**: The factory default is bidirectional. DIP switch 2 on the back controls this:
   - Switch UP (OFF): Bidirectional or unidirectional operation possible (default)
   - Switch DOWN (ON): Bidirectional operation only

3. **Receivers must be paired**: The pairing process (page 27-29) establishes the relationship between sender and receiver. A receiver will only accept commands from paired senders, and relay appears to only work within the same paired network.

4. **No explicit relay configuration**: The manual contains no menu, setting, or procedure for configuring relay behavior. It is entirely automatic.

### For ESP32/esphome-elero

Based on the official documentation, the ESP32 implementation needs:
- `hop` set to a non-zero value (default `0x0a` is correct) to enable relay participation
- The ESP32 must impersonate a paired remote (same address, correct rolling counter)
- At least one bidirectional receiver must be within direct RF range of the ESP32 for relay to work

---

## Troubleshooting Clues from Manual

### Status LED Indicators (Page 12-13)

| Status LED | Meaning |
|---|---|
| Orange then green | Bidirectional channel, receiver received the signal |
| Orange then red blinking | Bidirectional channel, one receiver did NOT receive the signal |
| Red then green | Bidirectional channel, signal received, batteries low |
| Red then red blinking | Bidirectional channel, one receiver did not receive, batteries low |
| Green | Unidirectional channel: signal sent (no confirmation possible) |

The "orange then red blinking" status is particularly relevant -- it indicates the TempoTel 2 knows when a receiver did not acknowledge the command, even through the relay chain. This confirms end-to-end acknowledgment works through the mesh.

### Battery Impact on Range (Page 13)

> "Die Sendeleistung bzw. Funkreichweite wird durch abnehmende Batterieleistung reduziert."
> (Transmit power and radio range are reduced by declining battery performance.)

When batteries drop below 2V, the device stops functioning entirely. Even before that point, reduced battery voltage means reduced transmit power and shorter range. This can cause intermittent range issues that gradually worsen.

### DIP Switch Settings (Page 34)

- **DIP Switch 1**: OEM setting (do not change)
- **DIP Switch 2**:
  - UP (OFF): Bidirectional or unidirectional operation possible (factory default)
  - DOWN (ON): Bidirectional operation only

---

## Sources

### Official Elero Documentation

1. **TempoTel 2 Bedienungsanleitung (User Manual)**
   - Document: 18 202.0002_DE_0922
   - Download: https://www.elero.de/de/downloads-service/downloads (search "TempoTel")
   - Publisher: elero GmbH, 73278 Schlierbach, Germany
   - Key pages: 7 (technical data), 10 (relay description), 12-13 (status LEDs), 34 (DIP switches), 36 (troubleshooting)

2. **Elero Product Catalog - Controls for Tubular Motors**
   - URL: https://www.elero.de/de/produkte/steuerungen-fuer-rohrantriebe/
   - Contains the ProLine 2 routing description
   - 45 products listed, no dedicated repeater product found

3. **Elero Company Website**
   - URL: https://www.elero.com / https://www.elero.de
   - Manufacturer: elero GmbH, Maybachstr. 30, 73278 Schlierbach, Germany
   - Contact: info@elero.de, +49 7021 9539-0

### Reverse Engineering Sources (Companion Document)

See `elero-relay-research-reverseeng.md` in this same directory for protocol-level details from:
- https://github.com/andyboeh/esphome-elero (primary ESPHome implementation)
- https://github.com/QuadCorei8085/elero_protocol (protocol reverse engineering)
- https://github.com/stanleypa/eleropy (Python implementation)
- GitHub issues #5, #11, #14, #15, #18, #23, #24, #29 (protocol insights)

### FCC Information

- **FCC ID search attempted**: 2ABSN-TEMPOTTEL2 -- no valid FCC filing found at https://fccid.io/2ABSN-TEMPOTTEL2
- The TempoTel 2 is primarily a European product (868 MHz). The 915 MHz variant for USA/Canada/Australia is mentioned in the manual but FCC filings were not locatable.
- **EU Conformity**: The manual (page 36) references directive 2014/53/EU and states the full declaration is available at www.elero.de/downloads-service

### GitHub Issue Insights Referenced

| Issue | URL | Relevant Finding |
|---|---|---|
| #18 | https://github.com/andyboeh/esphome-elero/issues/18 | Maintainer confirms hop=0x00 disables relay |
| #24 | https://github.com/andyboeh/esphome-elero/issues/24 | Definitive: hop=0x00 disables relay, bwd/fwd differ when relayed |
| #29 | https://github.com/andyboeh/esphome-elero/issues/29 | Working vs non-working blinds differ in typ=0xca (relay response) vs typ=0x6a |
