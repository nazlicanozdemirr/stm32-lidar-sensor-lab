"""
LiDAR Sensör Ölçüm Hızı Testi
Nucleo F446RE üzerinden UART1 ile bağlı LiDAR sensörünün ölçüm hızını test eder.
"""

import serial
import time
import sys
from collections import deque

# ==================== AYARLAR ====================
COM_PORT = "CO10"       
   # Nucleo'nun COM portu (Aygıt Yöneticisi'nden kontrol et)
BAUDRATE = 115200          # Nucleo'nun USB-UART baudrate'i
TEST_SURESI = 10           # Test süresi (saniye)
# =================================================

def com_port_bul():
    """Kullanılabilir COM portlarını listeler"""
    import serial.tools.list_ports
    ports = serial.tools.list_ports.comports()
    print("\nKullanılabilir COM portları:")
    for port in ports:
        print(f"  - {port.device}: {port.description}")
    print()

def lidar_hiz_testi(com_port, baudrate, sure_s):
    """
    LiDAR sensörünün ölçüm hızını test eder.
    
    Beklenen format: Her satırda bir ölçüm
    Örnek: "1234,567\r\n" (timestamp,mesafe)
    veya sadece: "567\r\n" (mesafe)
    """
    try:
        ser = serial.Serial(com_port, baudrate, timeout=1)
        print(f"✓ {com_port} portu {baudrate} bps ile açıldı.")
        print(f"✓ {sure_s} saniye boyunca ölçüm yapılıyor...\n")
        
        # Buffer temizle
        ser.reset_input_buffer()
        time.sleep(0.5)  # Sensörün hazır olması için bekle
        
        # Ölçüm değişkenleri
        baslangic = time.time()
        bitis = baslangic + sure_s
        toplam_olcum = 0
        
        # Anlık hız hesaplama için (son 1 saniyelik pencere)
        son_1_saniye_olcumler = deque()
        son_1_saniye_zaman = deque()
        
        # İstatistikler
        ilk_olcum_zamani = None
        son_olcum_zamani = None
        
        print("Ölçüm başladı... (Ctrl+C ile durdurabilirsiniz)\n")
        
        while time.time() < bitis:
            try:
                # Satır oku
                line = ser.readline()
                
                if line:
                    # Satırı decode et
                    try:
                        line_str = line.decode('utf-8', errors='ignore').strip()
                        
                        # Boş satırları atla
                        if not line_str:
                            continue
                        
                        # COUNT mesajlarını atla (debug mesajları)
                        if line_str.startswith("COUNT:"):
                            continue
                        
                        # Ölçüm sayacını artır
                        toplam_olcum += 1
                        simdiki_zaman = time.time()
                        
                        # İlk ve son ölçüm zamanlarını kaydet
                        if ilk_olcum_zamani is None:
                            ilk_olcum_zamani = simdiki_zaman
                        son_olcum_zamani = simdiki_zaman
                        
                        # Son 1 saniyelik pencereyi güncelle
                        son_1_saniye_olcumler.append(simdiki_zaman)
                        son_1_saniye_zaman.append(simdiki_zaman)
                        
                        # 1 saniyeden eski ölçümleri temizle
                        while son_1_saniye_zaman and (simdiki_zaman - son_1_saniye_zaman[0] > 1.0):
                            son_1_saniye_zaman.popleft()
                            son_1_saniye_olcumler.popleft()
                        
                        # Her 100 ölçümde bir anlık hızı göster
                        if toplam_olcum % 100 == 0:
                            anlik_hiz = len(son_1_saniye_olcumler)
                            gecen_sure = simdiki_zaman - baslangic
                            print(f"[{gecen_sure:6.2f}s] Toplam: {toplam_olcum:6d} | "
                                  f"Anlık hız: {anlik_hiz:5.1f} Hz | "
                                  f"Ortalama: {toplam_olcum/gecen_sure:5.1f} Hz", end='\r')
                    
                    except UnicodeDecodeError:
                        # Binary veri geliyorsa atla veya farklı şekilde işle
                        continue
                        
            except serial.SerialException as e:
                print(f"\nHata: {e}")
                break
        
        # Test bitti
        ser.close()
        
        # Sonuçları hesapla
        gecen_sure = son_olcum_zamani - ilk_olcum_zamani if ilk_olcum_zamani else time.time() - baslangic
        
        if toplam_olcum == 0:
            print("\n\n⚠ Hiç ölçüm alınamadı!")
            print("Kontrol edin:")
            print("  1. COM portu doğru mu?")
            print("  2. Nucleo kartı çalışıyor mu?")
            print("  3. Sensör bağlantısı doğru mu?")
            return
        
        ortalama_hiz = toplam_olcum / gecen_sure if gecen_sure > 0 else 0
        
        # Sonuçları göster
        print("\n" + "="*60)
        print("TEST SONUÇLARI")
        print("="*60)
        print(f"Toplam ölçüm sayısı:     {toplam_olcum}")
        print(f"Geçen süre:              {gecen_sure:.3f} saniye")
        print(f"Ortalama ölçüm hızı:     {ortalama_hiz:.2f} Hz")
        print(f"Minimum ölçüm periyodu:  {1000/ortalama_hiz:.2f} ms" if ortalama_hiz > 0 else "N/A")
        print("="*60)
        
        return ortalama_hiz
        
    except serial.SerialException as e:
        print(f"\n❌ Seri port hatası: {e}")
        print("\nKullanılabilir portları görmek için:")
        print("  python lidar_speed_test.py --list-ports")
        return None
    
    except KeyboardInterrupt:
        print("\n\n⚠ Test kullanıcı tarafından durduruldu.")
        ser.close()
        return None
    
    except Exception as e:
        print(f"\n❌ Beklenmeyen hata: {e}")
        return None


def main():
    """Ana fonksiyon"""
    print("="*60)
    print("LiDAR Sensör Ölçüm Hızı Testi")
    print("="*60)
    
    # Port listesi istenirse göster
    if len(sys.argv) > 1 and sys.argv[1] == "--list-ports":
        com_port_bul()
        return
    
    # COM port kontrolü
    if COM_PORT.startswith("COM"):
        print(f"Kullanılacak port: {COM_PORT}")
        print(f"Baudrate: {BAUDRATE}")
        print(f"Test süresi: {TEST_SURESI} saniye\n")
    else:
        print("⚠ Lütfen COM_PORT değişkenini ayarlayın!")
        com_port_bul()
        return
    
    # Testi başlat
    hiz = lidar_hiz_testi(COM_PORT, BAUDRATE, TEST_SURESI)
    
    if hiz:
        print(f"\n✓ Test tamamlandı. Ölçüm hızı: {hiz:.2f} Hz")


if __name__ == "__main__":
    main()
