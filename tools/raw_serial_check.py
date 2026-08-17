#!/usr/bin/env python3
"""
Porttan gelen TÜM veriyi ham olarak yazdırır.
STM32'den gerçekten veri geliyor mu anlamak için kullanın.
Kullanım: python raw_serial_check.py COM10
Ctrl+C ile çıkın.
"""

import sys
import serial
import serial.tools.list_ports

if len(sys.argv) < 2:
    print("Kullanım: python raw_serial_check.py COMx")
    print("Mevcut portlar:")
    for p in serial.tools.list_ports.comports():
        print(f"  {p.device}  - {p.description}")
    sys.exit(1)

port = sys.argv[1]
baud = int(sys.argv[2]) if len(sys.argv) > 2 else 9600

try:
    ser = serial.Serial(port, baud, timeout=0.5)
except Exception as e:
    print(f"Port açılamadı: {e}")
    sys.exit(1)

print(f"{port} @ {baud} açıldı. Veri bekleniyor... (Ctrl+C = çıkış)")
print("-" * 50)
byte_count = 0

try:
    while True:
        if ser.in_waiting:
            data = ser.read(ser.in_waiting)
            byte_count += len(data)
            # Hem ham bayt hem de metin olarak göster
            try:
                text = data.decode("utf-8", errors="replace")
                print(text, end="", flush=True)
            except Exception:
                print(data.hex(), end=" ", flush=True)
        else:
            import time
            time.sleep(0.05)
except KeyboardInterrupt:
    print(f"\n\nToplam {byte_count} bayt alındı.")
finally:
    ser.close()
