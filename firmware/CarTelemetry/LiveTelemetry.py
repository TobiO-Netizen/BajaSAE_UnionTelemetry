"""
LiveTelemetry.py — Real-time car telemetry dashboard

Reads LoRa receiver serial data and plots live subplots for
speed, temperature, acceleration, gyroscope, RSSI, and SNR.

Requirements:
    pip install pyserial matplotlib

Usage:
    python LiveTelemetry.py COM4            # Windows
    python LiveTelemetry.py /dev/ttyUSB0    # Linux/Mac
    python LiveTelemetry.py COM4 300        # custom rolling window
"""

import sys
import serial
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import time

# ── Config ─────────────────────────────────────────────────────
BAUD = 115200
WINDOW = 200           # default rolling window size (samples)

# Payload field indices (pipe-delimited)
# pkt|aX|aY|aZ|gX|gY|gZ|spdMph|objF|forceN|lat|lon|sats
F_PKT, F_AX, F_AY, F_AZ = 0, 1, 2, 3
F_GX, F_GY, F_GZ = 4, 5, 6
F_SPEED, F_TEMP, F_FORCE = 7, 8, 9
F_LAT, F_LON, F_SATS = 10, 11, 12
MIN_FIELDS = 13


def parse_rcv(line):
    """Parse +RCV=<addr>,<len>,<payload>,<RSSI>,<SNR>"""
    line = line.strip()
    if not line.startswith("+RCV="):
        return None

    body = line[5:]          # strip "+RCV="
    parts = body.split(",")
    if len(parts) < 5:
        return None

    payload = parts[2]
    rssi = int(parts[-2])
    snr = int(parts[-1])
    return payload, rssi, snr


def main():
    if len(sys.argv) < 2:
        print("Usage: python LiveTelemetry.py <COM_PORT> [window_size]")
        print("  e.g. python LiveTelemetry.py COM4")
        sys.exit(1)

    port = sys.argv[1]
    window = int(sys.argv[2]) if len(sys.argv) > 2 else WINDOW

    # ── Open serial ────────────────────────────────────────────
    print(f"Opening {port} at {BAUD} baud...")
    try:
        ser = serial.Serial(port, BAUD, timeout=2)
    except serial.SerialException as e:
        print(f"Error: {e}")
        sys.exit(1)
    print("Connected. Waiting for data...\n")

    # ── Buffers ────────────────────────────────────────────────
    t_buf    = deque(maxlen=window)
    ax_buf   = deque(maxlen=window)
    ay_buf   = deque(maxlen=window)
    az_buf   = deque(maxlen=window)
    gx_buf   = deque(maxlen=window)
    gy_buf   = deque(maxlen=window)
    gz_buf   = deque(maxlen=window)
    spd_buf  = deque(maxlen=window)
    tmp_buf  = deque(maxlen=window)
    frc_buf  = deque(maxlen=window)
    rssi_buf = deque(maxlen=window)
    snr_buf  = deque(maxlen=window)
    sat_buf  = deque(maxlen=window)

    start_time = time.time()

    # ── Figure setup ───────────────────────────────────────────
    plt.style.use('dark_background')
    fig, axes = plt.subplots(4, 3, figsize=(16, 10))
    fig.suptitle('Live Car Telemetry', fontsize=16, fontweight='bold')
    fig.subplots_adjust(hspace=0.45, wspace=0.30)

    plot_cfg = [
        # (row, col, buffer, label, unit, color)
        (0, 0, ax_buf,   'Accel X',     'g',    '#40CCE6'),
        (0, 1, ay_buf,   'Accel Y',     'g',    '#66D980'),
        (0, 2, az_buf,   'Accel Z',     'g',    '#F28C40'),
        (1, 0, gx_buf,   'Gyro X',      '°/s',  '#40CCE6'),
        (1, 1, gy_buf,   'Gyro Y',      '°/s',  '#66D980'),
        (1, 2, gz_buf,   'Gyro Z',      '°/s',  '#F28C40'),
        (2, 0, spd_buf,  'Speed',       'mph',  '#F24D59'),
        (2, 1, tmp_buf,  'Object Temp', '°F',   '#FFC033'),
        (2, 2, frc_buf,  'Force',       'N',    '#B373F2'),
        (3, 0, rssi_buf, 'RSSI',        'dBm',  '#E65989'),
        (3, 1, snr_buf,  'SNR',         'dB',   '#59BFF2'),
        (3, 2, sat_buf,  'Satellites',  '#',    '#80E666'),
    ]

    lines = []
    for r, c, _, label, unit, color in plot_cfg:
        ax = axes[r][c]
        ln, = ax.plot([], [], color=color, linewidth=1.2)
        ax.set_title(label, fontsize=11, color='white')
        ax.set_ylabel(unit, fontsize=9, color='#B0B0B0')
        ax.tick_params(colors='#808080', labelsize=8)
        ax.grid(True, alpha=0.3)
        lines.append(ln)

    status_text = fig.text(0.50, 0.01, 'Waiting for first packet...',
                           ha='center', fontsize=10, color='#B0B0B0',
                           family='monospace')

    # ── Animation update ───────────────────────────────────────
    def update(frame):
        try:
            raw = ser.readline().decode('utf-8', errors='ignore')
        except Exception:
            return lines

        result = parse_rcv(raw)
        if result is None:
            return lines

        payload, rssi, snr = result
        fields = payload.split('|')
        if len(fields) < MIN_FIELDS:
            return lines

        try:
            elapsed = time.time() - start_time
            pkt   = int(fields[F_PKT])
            aX    = float(fields[F_AX])
            aY    = float(fields[F_AY])
            aZ    = float(fields[F_AZ])
            gX    = float(fields[F_GX])
            gY    = float(fields[F_GY])
            gZ    = float(fields[F_GZ])
            speed = float(fields[F_SPEED])
            temp  = float(fields[F_TEMP])
            force = float(fields[F_FORCE])
            lat   = float(fields[F_LAT])
            lon   = float(fields[F_LON])
            sats  = int(fields[F_SATS])
        except (ValueError, IndexError):
            return lines

        # Console output
        print(f"[#{pkt:04d}]  Accel: {aX:+.2f} {aY:+.2f} {aZ:+.2f} g  |  "
              f"Gyro: {gX:+.1f} {gY:+.1f} {gZ:+.1f}  |  "
              f"Spd: {speed:.1f} mph  |  Temp: {temp:.1f}F  |  "
              f"Force: {force:.1f} N  |  "
              f"RSSI: {rssi}  SNR: {snr}  |  "
              f"Sats: {sats}  ({lat:.6f}, {lon:.6f})")

        # Push to buffers
        t_buf.append(elapsed)
        ax_buf.append(aX);    ay_buf.append(aY);    az_buf.append(aZ)
        gx_buf.append(gX);    gy_buf.append(gY);    gz_buf.append(gZ)
        spd_buf.append(speed); tmp_buf.append(temp); frc_buf.append(force)
        rssi_buf.append(rssi); snr_buf.append(snr); sat_buf.append(sats)

        # Update plot lines
        all_bufs = [ax_buf, ay_buf, az_buf,
                    gx_buf, gy_buf, gz_buf,
                    spd_buf, tmp_buf, frc_buf,
                    rssi_buf, snr_buf, sat_buf]

        for i, (r, c, buf, _, _, _) in enumerate(plot_cfg):
            lines[i].set_data(list(t_buf), list(buf))
            ax = axes[r][c]
            if len(t_buf) > 1:
                ax.set_xlim(t_buf[0], t_buf[-1])
            if len(buf) > 0:
                bmin, bmax = min(buf), max(buf)
                margin = max(abs(bmax - bmin) * 0.15, 0.5)
                ax.set_ylim(bmin - margin, bmax + margin)

        status_text.set_text(
            f'Pkt #{pkt}  |  Lat: {lat:.6f}  Lon: {lon:.6f}  |  '
            f'Sats: {sats}  |  RSSI: {rssi} dBm  |  SNR: {snr} dB  |  '
            f'Elapsed: {elapsed:.0f}s')

        return lines

    ani = animation.FuncAnimation(fig, update, interval=50, blit=False,
                                  cache_frame_data=False)
    plt.show()

    ser.close()
    print("\nWindow closed — stopped.")


if __name__ == "__main__":
    main()
