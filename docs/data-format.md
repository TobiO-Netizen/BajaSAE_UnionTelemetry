# Data Format Specification

## SD card CSV log (`data.csv`)

| Field        | Type   | Unit    | Description        |
|--------------|--------|---------|--------------------|
| Frame ID     | uint32 | —       | Sequential counter |
| Timestamp    | float  | seconds | Elapsed since boot |
| Speed        | float  | km/h    | GPS ground speed   |
| Acceleration | float  | km/h²   | IMU X-axis         |
| Strain       | float  | m/m     | Bridge output      |
| CVT temp     | float  | °F      | IR measurement     |

Log files auto-increment (`TEL_001.csv`, `TEL_002.csv`, …) across power cycles so previous logs
are never overwritten. Files are compatible with MATLAB `readtable()`, Excel, and Python
(pandas).

## Data rates

Sensors are sampled at their native rates, then adjusted for storage and transmission:

- **SD card write rate:** ~375 Bps (5 Hz sampling for most channels; GPS-derived fields carried
  over 5 readings per second since GPS caps at 1 Hz).
- **LoRa transmit rate:** ~99 bytes/packet at ~0.5–1 Hz (well under the 190 Bps design target and
  the module's 37.5 kbps capacity).

GPS fields (speed, latitude, longitude, satellite count) update at a maximum of 1 Hz, which
bottlenecks the effective system rate. The design compromises by sampling at up to 5 Hz and
carrying over the most recent GPS values between fixes.

## LoRa packet format

The receiver module emits each incoming transmission as one line:

```
+RCV=<senderAddress>,<length>,<payload>,<RSSI>,<SNR>
```

The `payload` is a pipe-delimited string of thirteen fields:

| # | Field            | Unit  |
|---|------------------|-------|
| 1 | Packet number    | —     |
| 2 | Acceleration X   | g     |
| 3 | Acceleration Y   | g     |
| 4 | Acceleration Z   | g     |
| 5 | Gyroscope X      | °/s   |
| 6 | Gyroscope Y      | °/s   |
| 7 | Gyroscope Z      | °/s   |
| 8 | GPS speed        | mph   |
| 9 | Object temp      | °F    |
| 10| Strain force     | N     |
| 11| Latitude         | °     |
| 12| Longitude        | °     |
| 13| Satellite count  | —     |

`RSSI` (dBm) and `SNR` (dB) are appended by the receiver module as link-quality metrics.

### Example line

```
+RCV=1,85,47|0.02|-0.01|0.98|1.2|-0.5|0.3|45.3|78.2|12.50|42.814563|-73.941521|8,-65,10
```

## Receiver parsing

The receiver script opens a serial connection at 115200 baud and reads lines continuously. For
each line it:

1. Checks for the `+RCV=` prefix.
2. Extracts `RSSI` and `SNR` from the trailing comma-separated fields.
3. Splits the payload on the pipe delimiter to recover the sensor values.

Values are pushed into rolling buffers and plotted on a twelve-panel dashboard (three plots each
for acceleration and gyroscope, plus speed, temperature, force, RSSI, SNR, and satellite count).
A status bar shows the current packet number, GPS coordinates, and link quality. Plots
auto-scroll over a 60-second rolling window, and every packet is also printed to the console as a
formatted log.
