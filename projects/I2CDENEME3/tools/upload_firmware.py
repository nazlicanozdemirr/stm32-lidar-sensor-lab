#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
STM32F446RE bootloader: .hex / .bin yükleyici (UART, MD5).

- Yüklenecek dosyanın cihazda olup olmadığı MD5 hash ile kontrol edilir.
- Dosya zaten yüklüyse (aynı MD5) yükleme atlanır; yoksa UART üzerinden
  buffer'lar halinde (256 byte) STM32'nin uygulama sektörüne (0x08008000)
  gönderilir.
- Hex dosyası parse edilip 0x08008000'den itibaren binary üretilir;
  .bin doğrudan bu adrese yazılacak imaj kabul edilir.

Protokol: bootloader.h (CMD_CHECK, CMD_START, ACK, NAK, RSP_ALREADY, RSP_NEED_LOAD).
"""

import argparse
import hashlib
import serial
import struct
import sys
import time

# Bootloader protokolü (bootloader.h ile aynı)
CMD_START = 0x01
CMD_CHECK = 0x02
ACK = 0x06
NAK = 0x15
RSP_ALREADY = 0x01   # Zaten yüklü → yükleme atlansın
RSP_NEED_LOAD = 0x00 # Yükleme gerekli

APP_START = 0x08008000
APP_MAX_SIZE = 480 * 1024

# Hash (MD5) bölgesi: bootloader ile aynı (ilk 512 + 20 atla + 512 byte)
HASH_REGION1_LEN = 512
HASH_SKIP = 20
HASH_REGION2_LEN = 512

# UART ile gönderim buffer boyutu (STM tarafında FLASH_WRITE_CHUNK = 256)
CHUNK = 256


def compute_image_hash(data):
    """Bootloader ile aynı bölge: ilk 512 byte + 20 atla + 512 byte → MD5 (16 byte)."""
    pad = bytes([0xFF])
    part1 = (data[:HASH_REGION1_LEN] + pad * HASH_REGION1_LEN)[:HASH_REGION1_LEN]
    start2 = HASH_REGION1_LEN + HASH_SKIP
    part2 = (data[start2:start2 + HASH_REGION2_LEN] + pad * HASH_REGION2_LEN)[:HASH_REGION2_LEN]
    return hashlib.md5(part1 + part2).digest()


def hash_to_hex(h):
    """16 byte MD5 → 32 karakter hex string."""
    return h.hex()


def hex_to_hash(s):
    """32 karakter hex string → 16 byte."""
    s = s.strip().replace(" ", "").lower()
    if len(s) != 32 or not all(c in "0123456789abcdef" for c in s):
        return None
    return bytes.fromhex(s)


def load_hex(path):
    """Intel HEX dosyasını okuyup 0x08008000'den itibaren binary döndür."""
    mem = {}
    ext_linear = 0
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            line = line.strip()
            if not line or not line.startswith(":"):
                continue
            raw = bytes.fromhex(line[1:])
            if len(raw) < 5:
                continue
            count, addr_hi, addr_lo, rec_type = raw[0], raw[1], raw[2], raw[3]
            addr = (addr_hi << 8) | addr_lo
            if rec_type == 0x00:  # Data
                base = (ext_linear << 16) | addr
                payload = raw[4:4 + count]
                for i, b in enumerate(payload):
                    mem[base + i] = b
            elif rec_type == 0x01:  # EOF
                break
            elif rec_type == 0x04:  # Extended Linear
                if count >= 2:
                    ext_linear = (raw[4] << 8) | raw[5]
    if not mem:
        return None, None
    max_addr = max(mem.keys())
    if max_addr < APP_START:
        return None, None
    size = max_addr - APP_START + 1
    out = bytearray(size)
    for i in range(size):
        out[i] = mem.get(APP_START + i, 0xFF)
    return bytes(out), APP_START


def load_bin(path):
    with open(path, "rb") as f:
        return f.read(), APP_START


def main():
    ap = argparse.ArgumentParser(
        description="STM32F446RE bootloader: .hex/.bin UART ile yükle (MD5 ile dosya varsa atla)."
    )
    ap.add_argument("file", help=".hex veya .bin dosyası")
    ap.add_argument("-p", "--port", default="COM10", help="Seri port (varsayılan: COM10)")
    ap.add_argument("-b", "--baud", type=int, default=115200, help="Baud (varsayılan: 115200)")
    ap.add_argument(
        "--force",
        action="store_true",
        help="MD5 kontrolü yapma, her zaman yükle",
    )
    ap.add_argument(
        "--md5-file",
        metavar="FILE",
        help="Harici .md5 dosyası (32 karakter hex); CHECK için bu hash kullanılır (yoksa imajdan hesaplanır).",
    )
    ap.add_argument(
        "--save-md5",
        metavar="FILE",
        nargs="?",
        const="",
        help="Hesaplanan MD5'i dosyaya yaz (argüman verilmezse <firmware>.md5).",
    )
    args = ap.parse_args()

    path = args.file
    if path.lower().endswith(".hex"):
        data, base = load_hex(path)
    elif path.lower().endswith(".bin"):
        data, base = load_bin(path)
    else:
        print("Dosya .hex veya .bin olmalı.", file=sys.stderr)
        sys.exit(1)

    if not data:
        print("Dosya okunamadı veya boş.", file=sys.stderr)
        sys.exit(1)

    size = len(data)
    if size > APP_MAX_SIZE:
        print(f"Dosya {APP_MAX_SIZE // 1024} KB'dan büyük.", file=sys.stderr)
        sys.exit(1)

    # Hash: harici .md5 dosyası varsa onu kullan, yoksa imajdan hesapla
    if args.md5_file:
        try:
            with open(args.md5_file, "r", encoding="utf-8") as f:
                line = f.readline()
            img_hash = hex_to_hash(line)
            if img_hash is None:
                print("Geçersiz .md5 dosyası (32 karakter hex olmalı).", file=sys.stderr)
                sys.exit(1)
        except Exception as e:
            print(f"MD5 dosyası okunamadı: {e}", file=sys.stderr)
            sys.exit(1)
    else:
        img_hash = compute_image_hash(data)

    if args.save_md5 is not None:
        out_path = args.save_md5 if args.save_md5 else path + ".md5"
        with open(out_path, "w", encoding="utf-8") as f:
            f.write(hash_to_hex(img_hash) + "\n")
        print(f"MD5 kaydedildi: {out_path}")

    try:
        ser = serial.Serial(args.port, args.baud, timeout=2.0)
        time.sleep(0.3)  # Cihazın hazır olması için
        ser.reset_input_buffer()
        ser.reset_output_buffer()
    except Exception as e:
        print(f"Port açılamadı: {e}", file=sys.stderr)
        sys.exit(1)

    def read_and_echo(timeout_s=0.2):
        """Cihazdan gelen metni okuyup konsola yazdır (seri monitör gibi)."""
        ser.timeout = timeout_s
        buf = ser.read(256)
        if buf:
            try:
                print("[Cihaz]", buf.decode("ascii", errors="replace").strip())
            except Exception:
                pass
        ser.timeout = 2.0

    try:
        if not args.force:
            ser.write(bytes([CMD_CHECK]))
            ser.write(img_hash)
            rsp = ser.read(1)
            read_and_echo()  # "ALREADY" veya "NEED_LOAD" varsa konsolda görünsün
            if rsp and rsp[0] == RSP_ALREADY:
                print("Firmware zaten yüklü (MD5 eşleşti), yükleme atlanıyor.")
                return
            # RSP_NEED_LOAD veya timeout → yükle

        print("Yükleme başlıyor (buffer'lar halinde UART → STM32 sektör 0x08008000)...")
        ser.write(bytes([CMD_START]))
        ser.timeout = 5.0  # İlk ACK için daha uzun bekle
        ack = ser.read(1)
        ser.timeout = 2.0
        if ack != bytes([ACK]):
            print("Cihaz ACK vermedi.", file=sys.stderr)
            if ack:
                print("  Alınan byte: 0x%02X (ACK=0x06 bekleniyordu)" % ack[0], file=sys.stderr)
            else:
                print("  (Cihazdan veri gelmedi. Bootloader modunda mı? B1+reset deneyin.)", file=sys.stderr)
            sys.exit(1)
        ser.write(struct.pack("<I", size))
        if ser.read(1) != bytes([ACK]):
            print("Boyut ACK alınamadı.", file=sys.stderr)
            sys.exit(1)

        sent = 0
        while sent < size:
            chunk = data[sent:sent + CHUNK]
            ser.write(chunk)
            sent += len(chunk)
            if sent % (4 * 1024) == 0 or sent == size:
                print(f"  {sent}/{size} byte")

        rsp = ser.read(1)
        read_and_echo()  # "OK" varsa konsolda görünsün
        if rsp and rsp[0] == ACK:
            print("Yükleme tamamlandı.")
        else:
            print("Yükleme sonrası ACK alınamadı.", file=sys.stderr)
            sys.exit(1)
    finally:
        ser.close()


if __name__ == "__main__":
    main()
