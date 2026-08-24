# Bill of Materials & Power Budget

## Bill of materials

| Component          | Part          | Qty | Cost   |
|--------------------|---------------|-----|--------|
| Microcontroller    | Arduino Nano ESP32 (ABX00083) | 1 | $14 |
| CAN transceiver    | TJA1051T/3    | 2   | $3     |
| LoRa               | RYLR998       | 2   | $16    |
| GPS                | NEO-M8M       | 1   | $8     |
| IMU                | MPU6050       | 1   | $3     |
| IR temperature     | MLX90614      | 1   | $5     |
| Strain gauge       | BF350-3AA     | 1   | $2     |
| Display            | ILI9341       | 1   | $8     |
| SD module          | SPI breakout  | 1   | $1.50  |
| USB-UART           | CP2102        | 1   | $2     |
| SD card            | 32 GB         | 1   | $6     |
| OBD-II cable       | J1962         | 1   | $4     |
| **Total**          |               |     | **$78**|

Target retail is $99–129, which preserves margin while staying accessible to individuals and
student teams.

## Power consumption

| Component          | Current   | Notes        |
|--------------------|-----------|--------------|
| ESP32              | 40–50 mA  | Wi-Fi off    |
| MPU6050            | 3.9 mA    | Normal       |
| MLX90614           | 1.5 mA    | Continuous   |
| RYLR998 (TX)       | ~40 mA    | Peak         |
| ILI9341 controller | 4–6 mA    |              |
| ILI9341 backlight  | 80 mA     |              |
| BF350 strain       | <1 mA     | Resistive    |
| GPS                | 20 mA     |              |
| SD card            | 60 mA     |              |
| CAN ×2             | 10 mA     | Active       |
| **Total**          | **~250 mA** |            |

## Battery life

- From a 12 V car battery (50 Ah): 580+ hours.
- From the 10,000 mAh LiPo (kart): 8–10 hours measured.

Both exceed the 10-hour requirement. The system is powered by a single 3.7 V 10,000 mAh
lithium-polymer cell feeding a Type-C USB 5 V 2 A boost converter that supplies the regulated
5 V rail. Theoretical maximum runtime is ~25 hours; the gap to the observed 10+ hours is due to
boost-converter efficiency losses, voltage-dependent efficiency, and the protection circuit's
conservative 2.9 V cutoff leaving unusable energy in the cell.

Charging is 5 V 2 A over Type-C (~5–6 hours full recharge). Onboard protection covers
over-discharge, over-charge, and short-circuit. Four LEDs show remaining capacity in ~25%
increments. The battery can be swapped and recharged on the device.
