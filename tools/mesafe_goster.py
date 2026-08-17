# -*- coding: utf-8 -*-
"""
Sensorden gelen mesafe verilerini ekranda gosterir.
Format: t_ms,distance_mm  veya  distance_mm
Kullanim: python mesafe_goster.py COM10
Ctrl+C ile cikis.
"""

import sys
import serial

if len(sys.argv) < 2:
    print("Kullanim: python mesafe_goster.py COM10 [baud]")
    print("Ornek:   python mesafe_goster.py COM10 9600")
    sys.exit(1)

port = sys.argv[1]
baud = int(sys.argv[2]) if len(sys.argv) > 2 else 9600

try:
    ser = serial.Serial(port, baud, timeout=0.1)
except Exception as e:
    print("Port acilamadi: %s" % e)
    sys.exit(1)

print("Port: %s @ %s" % (port, baud))
print("Mesafe verileri (t_ms, distance_mm). Ctrl+C = cikis")
print("-" * 40)

try:
    while True:
        line = ser.readline()
        if not line:
            continue
        try:
            text = line.decode("utf-8", errors="ignore").strip()
        except Exception:
            continue
        if not text:
            continue
        # t_ms,distance_mm veya sadece distance_mm
        parts = text.split(",")
        if len(parts) == 2:
            try:
                t_ms = int(parts[0].strip())
                dist_mm = int(parts[1].strip())
                dist_cm = dist_mm / 10.0
                print("t=%s ms  mesafe=%s mm  (%.1f cm)" % (t_ms, dist_mm, dist_cm))
            except ValueError:
                print(text)
        elif len(parts) == 1:
            try:
                dist_mm = int(parts[0].strip())
                dist_cm = dist_mm / 10.0
                print("mesafe=%s mm  (%.1f cm)" % (dist_mm, dist_cm))
            except ValueError:
                print(text)
        else:
            print(text)
except KeyboardInterrupt:
    print("\nCikis.")
finally:
    ser.close()
