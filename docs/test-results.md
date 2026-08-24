# Subsystem Verification Summary

All eleven subsystems passed their verification tests and met or exceeded their engineering
specifications. Summaries below; see the capstone report for full procedures and analysis plans.

## LoRa communication
Two RYLR998 modules configured to matching parameters (address 1/2, network ID 5, 915 MHz,
SF9 / 125 kHz / CR 4/5 / preamble 12). Numbered 13-field packets sent at 2-second intervals over
increasing distance.

| Distance (km) | Packet loss (%) |
|---------------|-----------------|
| 0.25          | 1.3             |
| 0.5           | 11.11           |

The <5% loss target was not met at longer range with the stock antenna. The module's SMA
connector allows a better antenna to be fitted; a higher-gain antenna should recover range at the
cost of some power draw.

## MPU6050 (IMU)
Confirmed at I²C 0x68. After a 300-sample rest calibration, the accelerometer read ~X=0.00 g,
Y=0.00 g, Z=1.00 g and the gyro stayed within ±0.5°/s at rest. All axes responded correctly to
tilt, rotation, and shake. Performs within datasheet specs.

## MLX90614 (IR temperature)
Verified against ice water (32°F) and boiling water (212°F). Correctly reported ambient and
object temperatures and converted between °C, °F, and K. Coexisted on the shared I²C bus with
the MPU6050 without interference.

## GPS (NEO-6M)
NMEA parser correctly extracted position, speed, and altitude. GPS speed matched the vehicle
speedometer within tolerance at 20 and 30 mph. Position cross-checked against a smartphone
reference. 1 Hz update rate confirmed — the primary bottleneck in the sensor chain, but adequate
for logging at moderate speeds. Does not function indoors (needs line of sight to satellites).

## Micro SD card
Mounted reliably when initialized in the correct order relative to the display. Created
auto-incrementing CSV files with correct headers, no corruption across extended sessions, and
parsed cleanly in MATLAB. Key challenge: SPI bus contention with the ILI9341 — solved by letting
TFT_eSPI configure HSPI first, then referencing the same peripheral for the SD card without
re-calling `_spi.begin()`.

## OBD-II / CAN (ESP32 TWAI)
TWAI initialized at 500 kbps. ECU detected within 200 ms via a PID 0x00 query to broadcast
0x7DF, listening on 0x7E8–0x7EF. Fast PIDs (RPM 0x0C, speed 0x0D, throttle 0x11) polled every
cycle; slow PIDs (coolant 0x05) every fifth cycle, on a dedicated FreeRTOS core. RPM, speed,
throttle, and coolant matched the dashboard. Hot-plug detection switched screens within 5 s.
Dual-core separation kept CAN latency off the main loop.

## UI + ILI9341
Both SquareLine Studio screens rendered correctly after resolving SPI init order; color inversion
fixed with `tft.invertDisplay()`. Needle rotation tracked GPS speed accurately; labels updated at
5 Hz without flicker. Screen switching used a 300 ms fade. Backlight needs 5 V for adequate
brightness. Future: an IPS panel for better viewing angles in a vehicle.

## Battery system
DWEII Type-C 5 V 2 A boost converter delivered stable 5 V across the discharge cycle. Achieved
10+ hours continuous runtime, exceeding the 10-hour target. Four-LED gauge tracked capacity;
built-in protections (over-discharge 2.9 V, over-charge 4.2 V, short-circuit) functioned as
specified.

---

> This is a summary. Sensitive to safety and measurement caveats — treat the numbers here as
> the prototype-milestone results, not final production figures.
