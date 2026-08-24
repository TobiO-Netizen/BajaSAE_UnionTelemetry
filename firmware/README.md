# Firmware

ESP32 (Arduino Nano ESP32 / ESP32-S3) firmware for the on-board module.

## Design

The firmware uses a modular structure — each subsystem is a C++ class with its own header. A
central `MainSystem` orchestrator initializes modules in a defined sequence, runs non-blocking
timed loops for sensor reads, SD logging, and LoRa transmission, and drives the LVGL display.

OBD-II polling runs on FreeRTOS core 0; the main telemetry loop runs on core 1 and reads OBD
data through a mutex-protected snapshot, so CAN latency never blocks sensor reads or display
refresh.

## Toolchain

- Arduino IDE (or arduino-cli) with the ESP32 board package
- Libraries: TFT_eSPI, LVGL v8, MPU6050, Adafruit MLX90614, TinyGPS++ (or equivalent), HX711,
  RYLR998/LoRa AT-command handling
- UI generated in SquareLine Studio and exported to `ui_*` sources

## To add

Drop the firmware sources here (e.g. `src/`, `include/`, and the SquareLine `ui/` export). Add a
`platformio.ini` or an Arduino sketch entry point and document build/flash steps.
