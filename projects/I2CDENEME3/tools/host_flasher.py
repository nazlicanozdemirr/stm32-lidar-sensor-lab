#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
STM32F446RE UART bootloader — Host flasher (paket protokolü, MD5, .hex/.bin).

Paket formatı: [0xAA][CMD:1B][LEN:2B LE][DATA][CRC16:2B LE] — CRC16-CCITT.
"""

import argparse
import hashlib
import struct
import sys
import time

try:
    import serial
except ImportError:
    print("pyserial gerekli: pip install pyserial", file=sys.stderr)
    sys.exit(1)

try:
    from intelhex import IntelHex
except ImportError:
    IntelHex = None
    print("intelhex gerekli (hex için): pip install intelhex", file=sys.stderr)

# Protokol (bootloader.h ile aynı)
PKT_SYNC    = 0xAA
CMD_CHECK_MD5   = 0x01
CMD_START_FLASH = 0x02
CMD_SEND_DATA   = 0x03
CMD_END_FLASH   = 0x04
ACK         = 0x79
NACK        = 0x1F
ACK_SAME    = 0x80
ACK_DIFF    = 0x81

APP_START   = 0x08008000
APP_MAX_SIZE = 480 * 1024
CHUNK_SIZE  = 256

# CRC16-CCITT tablosu (polinom 0x1021, init 0xFFFF)
CRC16_TABLE = [
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0,
]


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc = ((crc << 8) ^ CRC16_TABLE[(crc >> 8) ^ b]) & 0xFFFF
    return crc


def build_packet(cmd: int, payload: bytes) -> bytes:
    length = len(payload)
    header = bytes([PKT_SYNC, cmd, length & 0xFF, (length >> 8) & 0xFF])
    body = header[1:] + payload  # CRC over CMD + LEN(2) + DATA
    crc = crc16_ccitt(body)
    return header + payload + struct.pack("<H", crc)


def load_hex(path: str):
    if IntelHex is None:
        raise RuntimeError("intelhex yüklü değil; .hex için: pip install intelhex")
    ih = IntelHex(path)
    try:
        end = ih.maxaddr()
    except Exception:
        return None, APP_START
    if end < APP_START:
        return None, APP_START
    data = ih.tobinstr(start=APP_START, end=end)
    return data, APP_START


def load_bin(path: str):
    with open(path, "rb") as f:
        data = f.read()
    return data, APP_START


def main():
    parser = argparse.ArgumentParser(
        description="STM32F446RE UART bootloader: .hex/.bin yükle (paket protokolü, MD5)."
    )
    parser.add_argument("file", help=".hex veya .bin dosyası")
    parser.add_argument("-p", "--port", default="COM10", help="Seri port (varsayılan: COM10)")
    parser.add_argument("-b", "--baud", type=int, default=115200, help="Baud (varsayılan: 115200)")
    parser.add_argument("--force", action="store_true", help="MD5 atla, her zaman yükle")
    args = parser.parse_args()

    path = args.file
    if path.lower().endswith(".hex"):
        data, _ = load_hex(path)
    elif path.lower().endswith(".bin"):
        data, _ = load_bin(path)
    else:
        print("Dosya .hex veya .bin olmalı.", file=sys.stderr)
        sys.exit(1)

    if not data:
        print("Dosya okunamadı veya boş.", file=sys.stderr)
        sys.exit(1)

    size = len(data)
    if size > APP_MAX_SIZE:
        print(f"Firmware {APP_MAX_SIZE // 1024} KB'dan büyük olamaz.", file=sys.stderr)
        sys.exit(1)

    img_md5 = hashlib.md5(data).digest()

    try:
        ser = serial.Serial(args.port, args.baud, timeout=2.0)
        time.sleep(0.3)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
    except Exception as e:
        print(f"Port açılamadı: {e}", file=sys.stderr)
        sys.exit(1)

    def read_response(timeout=2.0):
        ser.timeout = timeout
        r = ser.read(1)
        ser.timeout = 2.0
        return r[0] if r else None

    try:
        if not args.force:
            pkt = build_packet(CMD_CHECK_MD5, img_md5)
            ser.write(pkt)
            rsp = read_response(3.0)
            if rsp == ACK_SAME:
                print("Firmware zaten yüklü (MD5 eşleşti). Yükleme atlandı.")
                return
            if rsp != ACK_DIFF:
                print(f"Beklenmeyen yanıt: 0x{rsp:02X} (ACK_DIFF=0x81 bekleniyordu).", file=sys.stderr)
                sys.exit(1)

        print("Flash başlatılıyor...")
        ser.write(build_packet(CMD_START_FLASH, b""))
        if read_response(5.0) != ACK:
            print("Cihaz ACK vermedi (flash başlatılamadı).", file=sys.stderr)
            sys.exit(1)

        sent = 0
        chunk_count = (size + CHUNK_SIZE - 1) // CHUNK_SIZE
        for i in range(chunk_count):
            start = i * CHUNK_SIZE
            chunk = data[start:start + CHUNK_SIZE]
            if len(chunk) < CHUNK_SIZE:
                chunk = chunk + bytes(CHUNK_SIZE - len(chunk))
            crc = crc16_ccitt(chunk)
            payload = chunk + struct.pack("<H", crc)
            ser.write(build_packet(CMD_SEND_DATA, payload))
            rsp = read_response(2.0)
            if rsp != ACK:
                print(f"Chunk {i+1}/{chunk_count} NACK.", file=sys.stderr)
                sys.exit(1)
            sent += len(chunk)
            pct = 100 * sent // size if size else 100
            print(f"\r  {sent}/{size} byte ({pct}%)", end="", flush=True)
        print()

        fw_size_payload = struct.pack("<I", size)
        ser.write(build_packet(CMD_END_FLASH, fw_size_payload))
        rsp = read_response(5.0)
        if rsp == ACK:
            print("Yükleme tamamlandı. Cihaz uygulamaya geçiyor.")
        else:
            print(f"Son ACK alınamadı: 0x{rsp:02X}", file=sys.stderr)
            sys.exit(1)
    finally:
        ser.close()


if __name__ == "__main__":
    main()
