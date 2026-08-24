# Vehicular Akasha — Baja SAE / Multi-Vehicle Telemetry System

A self-contained, sub-$80 vehicle telemetry and data-acquisition platform built around an
Arduino Nano ESP32. It captures real-time data from both a vehicle's OBD-II bus **and** an
independent onboard sensor suite, shows it live on an embedded touchscreen, logs full-resolution
data to an SD card, and streams a summary over long-range LoRa to a pit-side receiver — no
cellular, internet, or subscription required.

The system runs in two modes automatically:

- **Standard mode** — plugs into any OBD-II vehicle (1996+) and reads engine RPM, speed,
  throttle position, and coolant temperature over the CAN bus.
- **Baja mode** — for custom off-road karts with no OBD-II bus, it synthesizes a
  standards-compliant CAN stream from its own sensors, giving telemetry to vehicles that
  otherwise have none.

> Capstone project (ECE 498) for the Union College Baja SAE program.
> Author: Jesutobi (Tobi) Onigbogi.

---

## Highlights

- **Dual data sources** — OBD-II (15+ PIDs) plus independent GPS, IMU, IR temperature, and
  strain sensing. Independent sensors cross-check the OBD-II bus and keep logging through CAN
  failures.
- **Long-range wireless** — RYLR998 LoRa at 915 MHz for pit-to-vehicle links over open terrain,
  with no recurring fees.
- **Live onboard display** — 2.8" ILI9341 TFT running a custom LVGL dashboard (SquareLine
  Studio), ≥30 FPS, with automatic screen switching based on OBD-II connection state.
- **Reliable local logging** — 32 GB microSD, CSV format, directly importable into Excel,
  MATLAB, or Python.
- **10+ hour runtime** — 3.7 V 10,000 mAh LiPo with a 5 V boost converter; ~250 mA total draw.
- **~$78 bill of materials** — undercuts commercial telematics ($240–600/yr) and racing
  telemetry ($500–2000+).

---

## Repository layout

```
.
├── README.md              You are here
├── LICENSE                MIT
├── docs/                  Design documentation distilled from the capstone report
│   ├── architecture.md      System design, data flow, module breakdown
│   ├── bill-of-materials.md BOM and power budget
│   ├── data-format.md       CSV log format and LoRa packet spec
│   ├── standards.md         Applicable standards and codes
│   └── test-results.md      Subsystem verification summary
├── firmware/              ESP32 firmware (source to be added)
│   └── README.md
├── pc-tools/              PC-side receiver / plotting scripts
│   └── README.md
└── hardware/              Schematics, PCB, pinout
    └── README.md
```

The full capstone report (with schematics, PCB layout, and figures) is the authoritative source
for everything summarized here.

---

## System at a glance

| Subsystem        | Part            | Role                                                     |
|------------------|-----------------|----------------------------------------------------------|
| Microcontroller  | Arduino Nano ESP32 (ESP32-S3) | Central controller; dual-core, native TWAI/CAN |
| OBD-II / CAN     | 2× TJA1051T/3   | Reads vehicle CAN; synthesizes CAN for non-standard karts |
| Wireless         | RYLR998 (LoRa)  | 915 MHz telemetry link, ~5–10 km line of sight           |
| GPS              | NEO-6M / NEO-M8M | Position, ground speed, UTC time (1 Hz)                  |
| IMU              | MPU6050         | 3-axis accel (±16 g) + gyro (±2000°/s)                    |
| CVT belt temp    | MLX90614        | Non-contact IR temperature, ±0.5°C                        |
| Structural strain| BF350-3AA + HX711 | Frame fatigue / overload sensing                        |
| Display          | ILI9341 2.8" TFT | LVGL dashboard, touch                                    |
| Storage          | 32 GB microSD   | CSV logging at 5 Hz                                       |
| Power            | 3.7 V 10 Ah LiPo + boost | Regulated 5 V rail, 10+ hr runtime               |

---

## Data flow

Sensors are sampled at their native rates, normalized into a uniform CSV time series, and fanned
out to four destinations in parallel:

1. **SD card** — full-resolution local log (survives power loss and link dropouts).
2. **Display** — live LVGL dashboard for the driver.
3. **LoRa** — downsampled summary to the pit-side receiver.
4. **USB serial** — mirror of the stream for debugging and PC-side logging/plots.

For non-standard vehicles, a CAN translation layer remaps sensor data into standards-compliant
CAN frames, so off-the-shelf analysis tools work on karts that have no factory bus.

See [`docs/architecture.md`](docs/architecture.md) for the full breakdown.

---

## Status

All eleven subsystems passed their verification tests and met or exceeded their engineering
specifications (see [`docs/test-results.md`](docs/test-results.md)). This repository corresponds to
the ECE 498 prototype milestone. ECE 499 work targets multi-vehicle testing, compliance
verification, a production enclosure, a cloud dashboard, and manufacturing documentation.

## License

Released under the MIT License — see [`LICENSE`](LICENSE). The project is intentionally
open-platform to avoid planned obsolescence and enable community contributions.
