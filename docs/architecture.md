# System Architecture

## Overview

Vehicular Akasha is a two-unit system:

- **On-board module** — houses the ESP32, sensor suite, dual CAN transceivers, ILI9341 display,
  SD card, and RYLR998 LoRa transceiver on a common 3.3 V rail.
- **Pit-stop receiver** — a simpler RYLR998 + CP2102 USB-UART adapter that plugs into a laptop.

The architecture requires no infrastructure, internet, or subscriptions.

## Level 0 — black box

**Inputs:** vehicle CAN bus, GPS signals, vehicle motion, 12 V power, Wi-Fi credentials, user
commands, CVT temperature, structural strain.

**Outputs:** visual display (ILI9341), PC serial data, LoRa packets, SD card logs (`.csv`), CAN
frames, status LEDs.

## Level 1 — decomposition

The system decomposes into modules centered on the ESP32. Sensors feed the controller, which
routes normalized CSV data to four outputs:

- Display over SPI
- SD card over the shared SPI bus (independent chip-select)
- LoRa over UART
- USB serial

Two CAN transceivers provide OBD-II input and standards-compliant CAN output for non-standard
vehicles.

## Processing pipeline

1. **Acquire** — each sensor is sampled at its native rate to preserve fidelity. High-frequency
   sensors (accelerometer, gyro) are read far more often than slow peripherals (temperature,
   strain).
2. **Normalize** — raw readings are consolidated into a uniform CSV record that reconciles the
   varying sample rates into a single time series.
3. **Persist** — the record is committed to the SD card for on-device storage that survives power
   cycles and connectivity loss.
4. **Display** — the LVGL dashboard refreshes continuously at ≥30 FPS.
5. **Translate (non-standard vehicles)** — a CAN translation layer remaps non-standard frames
   into a consistent internal format so the system interfaces with many vehicle architectures
   without per-platform firmware changes.
6. **Transmit** — telemetry is sent over LoRa via `AT+SEND`.
7. **Mirror** — a USB serial stream mirrors the data to a connected PC for debugging and
   external logging/analysis.

## Software structure

Each subsystem is encapsulated in its own C++ class with a header. A central `MainSystem`
orchestrator initializes all modules in a defined sequence, runs non-blocking timed loops for
sensor reads, SD logging, and LoRa transmission, and manages LVGL display updates. Subsystems
initialize, fail, and recover independently without compromising overall operation.

OBD-II polling runs on a dedicated FreeRTOS core (core 0) via `xTaskCreatePinnedToCore()`, with
the main telemetry loop on core 1 accessing OBD data through a mutex-protected snapshot. This
keeps CAN latency from affecting sensor polling or display refresh.

## Modules

### 1. OBD-II / CAN bus interface
Two TJA1051T/3 transceivers. `CAN_INPUT` reads differential CAN from OBD-II pins 6/14 and
converts to logic levels for the ESP32 TWAI controller (no external MCP2515 needed).
`CAN_OUTPUT` transmits sensor-derived CAN frames for non-standard vehicles. Mode 01 PID
requests at 500 kbps, 120 Ω termination.

### 2. Microcontroller — Arduino Nano ESP32
Central controller. Natively supports SPI, UART, I²C, and TWAI (CAN). Chosen over the Teensy
4.1 for built-in Wi-Fi/Bluetooth, native CAN, lower cost (~$10–15), lower power (40–50 mA), and
a large, familiar ecosystem. 240 MHz is sufficient since peripheral subsystems are the
bottleneck.

### 3. USB serial interface
PC-side `SerialRW.py` receives CSV frames and logs to `data.csv`; `GraphRPM.py` renders live
matplotlib plots with an auto-scrolling x-axis. Also used for firmware flashing and serial
debugging.

### 4. GPS (NEO-6M / NEO-M8M)
Satellite-derived position, velocity, and timing, independent of the vehicle. Chosen as the
primary speed source over IMU integration because it avoids accelerometer drift. 1 Hz update
rate is the primary bottleneck in the sensor chain.

### 5. IMU (MPU6050)
6-axis: 3-axis accelerometer (±16 g) and gyroscope (±2000°/s). Captures hard braking, cornering
loads, terrain impacts, rollovers. I²C at 0x68. Sampled at 100 Hz, downsampled to 1 Hz for LoRa
while retaining high-rate data on the SD card. ~3.9 mA active.

### 6. IR temperature (MLX90614)
Non-contact IR thermometer for CVT belt monitoring. IR object temperature (−70 to 380°C) plus
ambient die temperature in one I²C read at 0x5A. Factory-calibrated ±0.5°C, ~1.5 mA.

### 7. Strain gauge (BF350-3AA + HX711)
Foil resistive gauge bonded to the kart's tubular frame in a Wheatstone bridge, conditioned by
an HX711 amplifier. Detects frame fatigue and structural overload. 350 Ω nominal, gauge
factor ~2.0, output in micro-strain.

### 8. Display (ILI9341 2.8" TFT)
320×240 color touchscreen over SPI. Custom LVGL dashboard from SquareLine Studio with a
circular speed gauge, status bar, and screen switching. ≥30 FPS via DMA-accelerated SPI.
Resistive touch avoids vibration-prone physical buttons.

### 9. SD card storage
Primary archival medium — captures every data point even when LoRa drops packets or USB is
disconnected. MicroSD over SPI, sharing the display bus with an independent chip-select. Writes
at 200–500 KB/s, far above the ~375 B/s requirement.

### 10. LoRa (RYLR998)
915 MHz ISM transceiver for pit-to-vehicle comms. UART-connected, configured via AT commands.
Actual telemetry load is ~320 bps against 37.5 kbps capacity (<1% utilization). Matched receiver
at the pit uses a CP2102 USB-UART adapter.

### 11. PC application
MATLAB live-graphing over COM serial (requires CP2102 drivers), plus the Python serial tools.

## Pin assignments (prototype)

| Function            | Pin(s)                                              |
|---------------------|-----------------------------------------------------|
| I²C (MPU6050, MLX90614) | SDA = GPIO 11, SCL = GPIO 12                    |
| GPS (UART, 9600)    | RX = GPIO 2, TX = GPIO 1                             |
| Shared SPI (display + SD) | SCLK = GPIO 8, MOSI = GPIO 38, MISO = GPIO 21 |
| Display control     | CS = GPIO 5, DC = GPIO 7, RST = GPIO 6              |
| SD card             | CS = GPIO 17                                        |
| CAN / TWAI          | TX = GPIO 13, RX = GPIO 14                          |

> Note: pin values reflect the breadboard prototype and should be confirmed against the current
> firmware and PCB before reuse.

## LoRa parameters

| Parameter        | Transmitter | Receiver |
|------------------|-------------|----------|
| `AT+MODE=`       | 0           | 0        |
| `AT+ADDRESS=`    | 1           | 2        |
| `AT+NETWORKID=`  | 5           | 5        |
| `AT+BAND=`       | 915 MHz     | 915 MHz  |
| `AT+PARAMETER=`  | 9,7,1,12    | default  |
