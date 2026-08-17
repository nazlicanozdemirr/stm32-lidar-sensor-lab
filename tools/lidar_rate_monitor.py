# -*- coding: utf-8 -*-
"""
LiDAR olcum hizi monitoru.
Nucleo USB VCP (COM port) uzerinden gelen satirlari okur;
5 saniyelik sabit ve kayan pencerede Hz hesaplar.
Kullanim: python lidar_rate_monitor.py [COM_PORT]
Ornek:   python lidar_rate_monitor.py COM10
"""

from __future__ import print_function
import sys
import time
import argparse
from collections import deque

if sys.version_info[0] < 3:
    print("Python 3 gerekli (python3 veya py -3 kullanin).")
    sys.exit(1)

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("pyserial gerekli: pip install pyserial")
    sys.exit(1)


DEFAULT_PORT = "COM10"
BAUD = 9600
FIXED_WINDOW_S = 5.0
SLIDING_WINDOW_S = 2.0
PRINT_INTERVAL_S = 1.0


def parse_line(line):
    """Satir: 't_ms,distance_mm' veya 'distance_mm'. Gecerliyse (t_ms, dist) veya (None, dist), degilse None."""
    line = line.strip()
    if not line:
        return None
    parts = line.split(",")
    if len(parts) == 2:
        try:
            t_ms = int(parts[0].strip())
            dist = int(parts[1].strip())
            return (t_ms, dist)
        except ValueError:
            pass
    if len(parts) == 1:
        try:
            dist = int(parts[0].strip())
            return (None, dist)
        except ValueError:
            pass
    return None


def main():
    parser = argparse.ArgumentParser(description="LiDAR olcum hizi (Hz) monitoru")
    parser.add_argument("port", nargs="?", default=DEFAULT_PORT,
                        help="COM port (orn. COM10). Varsayilan: " + DEFAULT_PORT)
    parser.add_argument("-b", "--baud", type=int, default=BAUD,
                        help="Baud rate (varsayilan: %s)" % BAUD)
    parser.add_argument("-f", "--fixed-window", type=float, default=FIXED_WINDOW_S,
                        help="Sabit pencere (saniye)")
    parser.add_argument("-s", "--sliding-window", type=float, default=SLIDING_WINDOW_S,
                        help="Kayan pencere (saniye)")
    parser.add_argument("-d", "--debug", action="store_true",
                        help="Ham veriyi goster")
    parser.add_argument("-l", "--list-ports", action="store_true",
                        help="COM portlarini listele")
    args = parser.parse_args()

    if args.list_ports:
        try:
            ports = serial.tools.list_ports.comports()
            print("Mevcut portlar:")
            for p in ports:
                print("  %s  - %s" % (p.device, p.description))
            if not ports:
                print("  (Hic port bulunamadi)")
        except Exception as e:
            print("Port listesi alinamadi:", e)
        sys.exit(0)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.1)
    except serial.SerialException as e:
        print("Port acilamadi: %s" % args.port, file=sys.stderr)
        print(str(e), file=sys.stderr)
        sys.exit(1)

    print("Port: %s @ %s. Cikmak icin Ctrl+C." % (args.port, args.baud))
    print("current_rate_hz (kayan) | avg_rate_hz (sabit)")
    print("-" * 50)

    count_total = 0
    fixed_window_count = 0
    fixed_window_start = time.perf_counter()
    sliding_events = deque(maxlen=10000)
    last_print = fixed_window_start

    try:
        while True:
            line = ser.readline()
            if not line:
                now = time.perf_counter()
                if now - last_print >= PRINT_INTERVAL_S:
                    last_print = now
                    elapsed_fixed = now - fixed_window_start
                    avg_hz = (fixed_window_count / elapsed_fixed) if elapsed_fixed > 0 else 0.0
                    if elapsed_fixed >= args.fixed_window:
                        fixed_window_count = 0
                        fixed_window_start = now
                    while sliding_events and sliding_events[0][0] < now - args.sliding_window:
                        sliding_events.popleft()
                    current_hz = (len(sliding_events) / args.sliding_window) if args.sliding_window > 0 else 0.0
                    print("current_rate_hz=%.1f  avg_rate_hz=%.1f  (count=%d)" % (current_hz, avg_hz, count_total))
                time.sleep(0.01)
                continue

            try:
                text = line.decode("utf-8", errors="ignore")
            except Exception:
                continue

            if args.debug:
                print("[DEBUG] %s" % repr(text.strip()))

            parsed = parse_line(text)
            if parsed is None:
                continue

            count_total += 1
            fixed_window_count += 1
            now = time.perf_counter()
            sliding_events.append((now, 1))

    except KeyboardInterrupt:
        print("\nDurduruldu.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
