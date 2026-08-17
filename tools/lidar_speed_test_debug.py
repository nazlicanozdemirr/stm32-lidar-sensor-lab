"""
LiDAR Sensör Ölçüm Hızı Testi - Debug Modu
STM32 debug kodundan gelen verileri parse edip ölçüm hızını hesaplar.
"""

import serial
import time
import sys
import re
from collections import deque
from datetime import datetime

# ==================== AYARLAR ====================
COM_PORT = "COM10"        # Nucleo'nun COM portu (STMicroelectronics STLink Virtual COM Port)
BAUDRATE = 115200         # Nucleo'nun USB-UART baudrate'i
TEST_SURESI = 10          # Test süresi (saniye)
DEBUG_MODE = True         # True: Tüm debug mesajlarını göster, False: Sadece ölçümleri göster
# =================================================

class LiDARTest:
    def __init__(self, com_port, baudrate, test_suresi, debug_mode=True):
        self.com_port = com_port
        self.baudrate = baudrate
        self.test_suresi = test_suresi
        self.debug_mode = debug_mode
        
        # Ölçüm verileri
        self.olcumler = []  # (timestamp, mesafe) çiftleri
        self.paket_sayisi = 0
        self.byte_sayisi = 0
        
        # Anlık hız hesaplama için (son 1 saniyelik pencere)
        self.son_1_saniye_olcumler = deque()
        self.son_1_saniye_zaman = deque()
        
        # İstatistikler
        self.ilk_olcum_zamani = None
        self.son_olcum_zamani = None
        
    def parse_mesafe_satiri(self, line):
        """'timestamp,mesafe' formatındaki satırı parse et"""
        # Format: "12345,567\r\n" veya "12345,567"
        match = re.match(r'(\d+),(\d+)', line.strip())
        if match:
            timestamp = int(match.group(1))
            mesafe = int(match.group(2))
            return timestamp, mesafe
        return None, None
    
    def parse_mesafe_mesaji(self, line):
        """'MESAFE: XXX cm' formatındaki mesajı parse et"""
        match = re.search(r'MESAFE:\s*(\d+)\s*cm', line)
        if match:
            return int(match.group(1))
        return None
    
    def parse_paket_satiri(self, line):
        """'PAKET[...]: ...' formatındaki satırı parse et"""
        match = re.search(r'PAKET\[(\d+)\]:', line)
        if match:
            return int(match.group(1))
        return None
    
    def parse_istatistik_satiri(self, line):
        """İstatistik satırlarını parse et"""
        if 'Toplam paket:' in line:
            match = re.search(r'Toplam paket:\s*(\d+)', line)
            if match:
                return ('paket', int(match.group(1)))
        elif 'Toplam byte:' in line:
            match = re.search(r'Toplam byte:\s*(\d+)', line)
            if match:
                return ('byte', int(match.group(1)))
        elif 'Olcum hizi:' in line:
            match = re.search(r'Olcum hizi:\s*([\d.]+)\s*Hz', line)
            if match:
                return ('hiz', float(match.group(1)))
        return None
    
    def test_baslat(self):
        """Testi başlat"""
        try:
            ser = serial.Serial(self.com_port, self.baudrate, timeout=1)
            print(f"✓ {self.com_port} portu {self.baudrate} bps ile açıldı.")
            print(f"✓ {self.test_suresi} saniye boyunca ölçüm yapılıyor...\n")
            
            if self.debug_mode:
                print("="*70)
                print("DEBUG MODU: Tüm mesajlar gösterilecek")
                print("="*70 + "\n")
            else:
                print("NORMAL MOD: Sadece ölçüm verileri gösterilecek\n")
            
            # Buffer temizle
            ser.reset_input_buffer()
            time.sleep(0.5)
            
            baslangic = time.time()
            bitis = baslangic + self.test_suresi
            
            print("Ölçüm başladı... (Ctrl+C ile durdurabilirsiniz)\n")
            
            while time.time() < bitis:
                try:
                    line = ser.readline()
                    
                    if not line:
                        continue
                    
                    # Satırı decode et
                    try:
                        line_str = line.decode('utf-8', errors='ignore').strip()
                    except:
                        continue
                    
                    if not line_str:
                        continue
                    
                    # Debug mesajlarını göster veya filtrele
                    if self.debug_mode:
                        # Tüm mesajları göster
                        if line_str.startswith("RX["):
                            print(f"  {line_str}")
                        elif line_str.startswith("PAKET["):
                            print(f"\n{line_str}")
                            paket_len = self.parse_paket_satiri(line_str)
                            if paket_len:
                                self.paket_sayisi += 1
                        elif "MESAFE:" in line_str:
                            print(f"  {line_str}")
                            mesafe = self.parse_mesafe_mesaji(line_str)
                            if mesafe:
                                self.son_olcum_zamani = time.time()
                                if self.ilk_olcum_zamani is None:
                                    self.ilk_olcum_zamani = self.son_olcum_zamani
                        elif "ISTATISTIKLER" in line_str:
                            print(f"\n{line_str}")
                        elif line_str.startswith("---") or line_str.startswith("==="):
                            print(line_str)
                        elif "UYARI:" in line_str or "HATA:" in line_str:
                            print(f"⚠ {line_str}")
                        elif any(x in line_str for x in ["Toplam", "Olcum hizi"]):
                            print(f"  {line_str}")
                            istat = self.parse_istatistik_satiri(line_str)
                            if istat:
                                if istat[0] == 'paket':
                                    self.paket_sayisi = istat[1]
                                elif istat[0] == 'byte':
                                    self.byte_sayisi = istat[1]
                    else:
                        # Sadece ölçüm verilerini işle
                        if "MESAFE:" in line_str:
                            mesafe = self.parse_mesafe_mesaji(line_str)
                            if mesafe:
                                self.son_olcum_zamani = time.time()
                                if self.ilk_olcum_zamani is None:
                                    self.ilk_olcum_zamani = self.son_olcum_zamani
                    
                    # Ölçüm verisi satırını parse et (format: "timestamp,mesafe")
                    timestamp, mesafe = self.parse_mesafe_satiri(line_str)
                    if timestamp is not None and mesafe is not None:
                        simdiki_zaman = time.time()
                        
                        # İlk ölçüm zamanını kaydet
                        if self.ilk_olcum_zamani is None:
                            self.ilk_olcum_zamani = simdiki_zaman
                        
                        self.son_olcum_zamani = simdiki_zaman
                        
                        # Ölçümü kaydet
                        self.olcumler.append((simdiki_zaman, mesafe))
                        
                        # Son 1 saniyelik pencereyi güncelle
                        self.son_1_saniye_olcumler.append(simdiki_zaman)
                        self.son_1_saniye_zaman.append(simdiki_zaman)
                        
                        # 1 saniyeden eski ölçümleri temizle
                        while self.son_1_saniye_zaman and (simdiki_zaman - self.son_1_saniye_zaman[0] > 1.0):
                            self.son_1_saniye_zaman.popleft()
                            self.son_1_saniye_olcumler.popleft()
                        
                        # Her 50 ölçümde bir anlık hızı göster (normal modda)
                        if not self.debug_mode and len(self.olcumler) % 50 == 0:
                            anlik_hiz = len(self.son_1_saniye_olcumler)
                            gecen_sure = simdiki_zaman - baslangic
                            print(f"[{gecen_sure:6.2f}s] Toplam: {len(self.olcumler):6d} | "
                                  f"Anlık hız: {anlik_hiz:5.1f} Hz | "
                                  f"Ortalama: {len(self.olcumler)/gecen_sure:5.1f} Hz", end='\r')
                
                except serial.SerialException as e:
                    print(f"\nSeri port hatası: {e}")
                    break
                except Exception as e:
                    if self.debug_mode:
                        print(f"\nParse hatası: {e}")
                    continue
            
            # Test bitti
            ser.close()
            
            # Sonuçları hesapla ve göster
            self.sonuclari_goster(baslangic)
            
        except serial.SerialException as e:
            print(f"\n❌ Seri port hatası: {e}")
            print("\nKullanılabilir portları görmek için:")
            print("  python lidar_speed_test_debug.py --list-ports")
            return None
        
        except KeyboardInterrupt:
            print("\n\n⚠ Test kullanıcı tarafından durduruldu.")
            ser.close()
            self.sonuclari_goster(baslangic)
            return None
        
        except Exception as e:
            print(f"\n❌ Beklenmeyen hata: {e}")
            return None
    
    def sonuclari_goster(self, baslangic_zamani):
        """Test sonuçlarını göster"""
        print("\n\n" + "="*70)
        print("TEST SONUÇLARI")
        print("="*70)
        
        if len(self.olcumler) == 0:
            print("\n⚠ HİÇ ÖLÇÜM ALINAMADI!")
            print("\nKontrol edin:")
            print("  1. COM portu doğru mu?")
            print("  2. Nucleo kartı çalışıyor mu?")
            print("  3. Sensör bağlantısı doğru mu?")
            print("  4. STM32 kodunda 'LiDAR Test Started' mesajı geliyor mu?")
            return
        
        # Zaman hesaplamaları
        gecen_sure = self.son_olcum_zamani - self.ilk_olcum_zamani if self.ilk_olcum_zamani else time.time() - baslangic_zamani
        
        # Hız hesaplamaları
        ortalama_hiz = len(self.olcumler) / gecen_sure if gecen_sure > 0 else 0
        
        # Mesafe istatistikleri
        mesafeler = [m[1] for m in self.olcumler]
        if mesafeler:
            min_mesafe = min(mesafeler)
            max_mesafe = max(mesafeler)
            ortalama_mesafe = sum(mesafeler) / len(mesafeler)
        else:
            min_mesafe = max_mesafe = ortalama_mesafe = 0
        
        # Sonuçları göster
        print(f"\n📊 ÖLÇÜM İSTATİSTİKLERİ:")
        print(f"  Toplam ölçüm sayısı:     {len(self.olcumler):6d}")
        print(f"  Toplam paket sayısı:    {self.paket_sayisi:6d}")
        print(f"  Toplam byte sayısı:     {self.byte_sayisi:6d}")
        print(f"  Geçen süre:              {gecen_sure:8.3f} saniye")
        print(f"  Ortalama ölçüm hızı:     {ortalama_hiz:8.2f} Hz")
        
        if ortalama_hiz > 0:
            print(f"  Minimum ölçüm periyodu:  {1000/ortalama_hiz:8.2f} ms")
        
        print(f"\n📏 MESAFE İSTATİSTİKLERİ:")
        print(f"  Minimum mesafe:         {min_mesafe:6d} cm")
        print(f"  Maksimum mesafe:         {max_mesafe:6d} cm")
        print(f"  Ortalama mesafe:         {ortalama_mesafe:6.1f} cm")
        
        # Anlık hız analizi
        if len(self.son_1_saniye_olcumler) > 0:
            anlik_hiz = len(self.son_1_saniye_olcumler)
            print(f"\n⚡ ANLIK HIZ:")
            print(f"  Son 1 saniyedeki hız:   {anlik_hiz:6.1f} Hz")
        
        print("\n" + "="*70)
        
        # Ölçüm verilerini dosyaya kaydet (opsiyonel)
        self.verileri_kaydet()
    
    def verileri_kaydet(self):
        """Ölçüm verilerini dosyaya kaydet"""
        if len(self.olcumler) == 0:
            return
        
        dosya_adi = f"lidar_test_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        try:
            with open(dosya_adi, 'w') as f:
                f.write("Timestamp,Time(s),Distance(cm)\n")
                baslangic = self.olcumler[0][0]
                for timestamp, mesafe in self.olcumler:
                    gecen_sure = timestamp - baslangic
                    f.write(f"{timestamp:.3f},{gecen_sure:.3f},{mesafe}\n")
            print(f"\n✓ Ölçüm verileri '{dosya_adi}' dosyasına kaydedildi.")
        except Exception as e:
            print(f"\n⚠ Veriler kaydedilemedi: {e}")


def com_port_bul():
    """Kullanılabilir COM portlarını listeler"""
    import serial.tools.list_ports
    ports = serial.tools.list_ports.comports()
    print("\nKullanılabilir COM portları:")
    for port in ports:
        print(f"  - {port.device}: {port.description}")
    print()


def main():
    """Ana fonksiyon"""
    print("="*70)
    print("LiDAR Sensör Ölçüm Hızı Testi - Debug Modu")
    print("="*70)
    
    # Port listesi istenirse göster
    if len(sys.argv) > 1 and sys.argv[1] == "--list-ports":
        com_port_bul()
        return
    
    # COM port kontrolü
    if COM_PORT.startswith("COM"):
        print(f"Port: {COM_PORT}")
        print(f"Baudrate: {BAUDRATE}")
        print(f"Test süresi: {TEST_SURESI} saniye")
        print(f"Debug modu: {'Açık' if DEBUG_MODE else 'Kapalı'}\n")
    else:
        print("⚠ Lütfen COM_PORT değişkenini ayarlayın!")
        com_port_bul()
        return
    
    # Testi başlat
    test = LiDARTest(COM_PORT, BAUDRATE, TEST_SURESI, DEBUG_MODE)
    test.test_baslat()


if __name__ == "__main__":
    main()
