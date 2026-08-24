# PC Tools

Pit-side and desktop utilities for receiving and visualizing telemetry.

## Scripts (to add)

- **`SerialRW.py`** — opens the COM port at 115200 baud, reads incoming CSV/`+RCV=` frames, and
  logs them to `data.csv`.
- **`GraphRPM.py`** — reads the logged CSV and renders live matplotlib plots with an
  auto-scrolling x-axis.
- **MATLAB receiver** — parses `+RCV=` lines into a twelve-panel live dashboard (accel ×3,
  gyro ×3, speed, temperature, force, RSSI, SNR, satellites) over a 60-second rolling window.
- **`PlotTelemetry.m`** — reads SD-card CSV logs via `readtable()` for post-session analysis.

## Requirements

- Python 3 with `pyserial` and `matplotlib`
- CP2102 USB-UART drivers (bundled with some OSes; install manually otherwise)
- MATLAB for the live dashboard / post-processing scripts

See [`../docs/data-format.md`](../docs/data-format.md) for the packet and CSV specifications the
scripts parse.
