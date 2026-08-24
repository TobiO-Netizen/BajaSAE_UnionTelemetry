# Hardware

Schematics, PCB, and reference pinouts for the on-board module and pit-stop receiver.

## Contents (to add)

- **On-board module schematic** — ESP32, dual TJA1051T/3, ILI9341, SD, RYLR998, GPS, MPU6050,
  MLX90614, HX711 + BF350, boost converter. Designed in EasyEDA (V1.0).
- **PC receiver** — RYLR998 + CP2102 USB-UART adapter.
- **PCB layout** — 2-layer, common SMD/through-hole packages.
- **Pinout reference** — Arduino Nano ESP32 (ABX00083).

## Design constraints

- Read-only OBD-II (no ECU write commands).
- Reverse-polarity diodes, overcurrent fuses, ESD protection on CAN lines.
- Operating range −20°C to 70°C; vibration per IEC 60068-2-6; RoHS-compliant.
- Standard 2-layer PCB for manufacturability.

See [`../docs/architecture.md`](../docs/architecture.md) for the prototype pin assignments and
[`../docs/bill-of-materials.md`](../docs/bill-of-materials.md) for the BOM and power budget.
