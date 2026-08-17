"""
LiDAR Sensör Debug Testi
Bu script sensörden veri gelip gelmediğini kontrol eder.
"""

import serial
import time
import sys

# ==================== AYARLAR ====================
COM_PORT = "COM5"          # Nucleo'nun COM portu
BAUDRATE = 115200          # Baudrate
TEST_SURESI = 5            # 5 saniye test
# =================================================

def debug_test():
    """Ham veri okuma testi"""
    try:
        ser = serial.Serial(COM_PORT, BAUDRATE, timeout=1)
        print(f"✓ {COM_PORT} portu {baudrate} bps ile açıldı.")
        print(f"✓ {TEST_SURESI} saniye boyunca ham veri okunuyor...\n")
        print("="*60)
        
        ser.reset_input_buffer()
        time.sleep(0.5)
        
        baslangic = time.time()
        bitis = baslangic + TEST_SURESI
        sayac = 0
        
        print("Gelen veriler (ham):\n")
        
        while time.time() < bitis:
            # Byte byte oku
            if ser.in_waiting > 0:
                byte = ser.read(1)
                sayac += 1
                
                # ASCII karakter ise göster
                if 32 <= byte[0] <= 126:
                    print(f"[{sayac:4d}] ASCII: '{byte.decode('utf-8', errors='ignore')}' (0x{byte[0]:02X})")
                else:
                    print(f"[{sayac:4d}] HEX: 0x{byte[0]:02X}")
                
                # Satır satır da oku
                line = ser.readline()
                if line:
                    try:
                        line_str = line.decode('utf-8', errors='ignore').strip()
                        if line_str:
                            print(f"      Satır: '{line_str}'")
                    except:
                        pass
        
        ser.close()
        
        print("\n" + "="*60)
        print(f"Toplam {sayac} byte okundu.")
        
        if sayac == 0:
            print("\n⚠ HİÇ VERİ GELMEDİ!")
            print("\nKontrol edin:")
            print("1. COM portu doğru mu?")
            print("2. Nucleo kartı çalışıyor mu? (LED yanıyor mu?)")
            print("3. Sensör bağlantıları doğru mu?")
            print("4. STM32 kodunda 'LiDAR Test Started' mesajı geliyor mu?")
        else:
            print("✓ Veri geliyor! Sensör bağlantısı çalışıyor.")
            print("  Ancak paket formatı kodda beklenen formattan farklı olabilir.")
        
    except Exception as e:
        print(f"\n❌ Hata: {e}")
        print("\nKullanılabilir portları görmek için:")
        print("  python debug_test.py --list-ports")

def list_ports():
    """Kullanılabilir COM portlarını listele"""
    import serial.tools.list_ports
    ports = serial.tools.list_ports.comports()
    print("\nKullanılabilir COM portları:")
    for port in ports:
        print(f"  - {port.device}: {port.description}")
    print()

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--list-ports":
        list_ports()
    else:
        print("="*60)
        print("LiDAR Sensör Debug Testi")
        print("="*60)
        print(f"Port: {COM_PORT}")
        print(f"Baudrate: {BAUDRATE}\n")
        debug_test()
