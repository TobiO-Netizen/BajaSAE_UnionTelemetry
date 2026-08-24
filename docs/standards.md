# Standards & Codes

The system was designed against the following standards.

## SAE J1979 — OBD-II diagnostic test modes
Defines the Parameter IDs (PIDs) the system queries for engine RPM, vehicle speed, coolant
temperature, and other metrics. Mandated for every U.S. passenger vehicle since 1996, so the
system interfaces with virtually any car without manufacturer-specific adapters. Nine diagnostic
modes cover real-time data streaming, freeze-frame retrieval, and trouble-code reading.

## ISO 15765-4 and ISO 11898-2 — CAN protocol and physical layer
Govern how data moves between vehicle ECUs. ISO 11898 defines message formatting,
arbitration-based prioritization, and CRC error protection. The physical-layer spec (ISO 11898-2)
sets the electrical characteristics the TJA1051T/3 transceivers must meet: differential signaling
at 500 kbps with 120 Ω termination. These ensure reliable communication with CAN-equipped
vehicles and that synthesized CAN frames for non-standard vehicles are compliant with
off-the-shelf analysis tools.

## SAE J2284 — CAN signal quality
Specifies voltage levels, rise times, and common-mode rejection for CAN transceivers in
passenger-vehicle environments, keeping signals readable amid electrical noise from ignition
coils, alternators, and motors.

## FCC Part 15 / 15.247 — RF emissions
The RYLR998 operates at 915 MHz in the unlicensed ISM band. Under Part 15.247,
spread-spectrum devices are allowed up to 1 W (30 dBm) conducted output; the RYLR998 operates
well below this. Compliance allows deployment at competition venues and on public roads without
licensing.

## Baja SAE competition rules
All electrical systems must be securely mounted and protected from water, mud, and debris.
Wiring must be routed away from hot/moving components and be mechanically secure against
sustained vibration. Two rules directly shaped the design:

- The stock fuel tank cannot be modified → no fuel-level sensor.
- On-vehicle battery charging during operation is prohibited → drives the 10-hour minimum
  battery-life requirement.

## Other referenced standards
- IEC 60068-2-6 — environmental vibration testing.
- RoHS — materials compliance.
- GDPR / CCPA — data-privacy compliance; users retain explicit data control.
- NSPE Code of Ethics — professional-practice framework (see ethics section of the report).
