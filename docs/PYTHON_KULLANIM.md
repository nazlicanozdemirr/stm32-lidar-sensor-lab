# Python Hız Testi Scripti Kullanım Kılavuzu

## Kurulum

```bash
pip install pyserial
```

## Kullanım

### 1. Temel Kullanım

```bash
python lidar_speed_test_debug.py
```

### 2. COM Portunu Bulma

```bash
python lidar_speed_test_debug.py --list-ports
```

### 3. Ayarları Değiştirme

`lidar_speed_test_debug.py` dosyasının başındaki ayarları değiştir:

```python
COM_PORT = "COM5"          # Kendi COM portunu yaz
BAUDRATE = 115200         # Nucleo'nun baudrate'i
TEST_SURESI = 10          # Test süresi (saniye)
DEBUG_MODE = True         # True: Tüm mesajları göster, False: Sadece ölçümler
```

## Debug Modu vs Normal Mod

### Debug Modu (DEBUG_MODE = True)
- Tüm STM32 debug mesajlarını gösterir
- Her byte'ı gösterir (ilk 100 byte)
- Paket analizlerini gösterir
- İstatistikleri gösterir
- **Kullanım:** Sensörden veri gelip gelmediğini kontrol etmek için

### Normal Mod (DEBUG_MODE = False)
- Sadece ölçüm verilerini gösterir
- Her 50 ölçümde bir anlık hızı gösterir
- Daha temiz çıktı
- **Kullanım:** Sadece hız testi yapmak için

## Beklenen Çıktı

### Debug Modunda:
```
==============================================================
DEBUG MODU: Tüm mesajlar gösterilecek
==============================================================

  RX[1]: 0x59 ('Y')
  RX[2]: 0x59 ('Y')
  RX[3]: 0x01 ('.')
  
PAKET[9]: 59 59 01 23 45 67 89 AB CD | ASCII: YY.#Eg...
  MESAFE: 291 cm

--- ISTATISTIKLER ---
  Toplam paket: 125
  Toplam byte: 1125
  Olcum hizi: 125.00 Hz
----------------------
```

### Normal Modda:
```
[  5.23s] Toplam:    250 | Anlık hız: 125.0 Hz | Ortalama: 125.0 Hz
```

### Test Sonuçları:
```
======================================================================
TEST SONUÇLARI
======================================================================

📊 ÖLÇÜM İSTATİSTİKLERİ:
  Toplam ölçüm sayısı:       1250
  Toplam paket sayısı:       1250
  Toplam byte sayısı:        11250
  Geçen süre:              10.000 saniye
  Ortalama ölçüm hızı:     125.00 Hz
  Minimum ölçüm periyodu:    8.00 ms

📏 MESAFE İSTATİSTİKLERİ:
  Minimum mesafe:           150 cm
  Maksimum mesafe:           500 cm
  Ortalama mesafe:           325.5 cm

⚡ ANLIK HIZ:
  Son 1 saniyedeki hız:     125.0 Hz

======================================================================

✓ Ölçüm verileri 'lidar_test_20260212_143025.csv' dosyasına kaydedildi.
```

## Özellikler

1. **Otomatik Veri Parse:** STM32'den gelen tüm mesajları otomatik parse eder
2. **Hız Hesaplama:** Ortalama ve anlık ölçüm hızını hesaplar
3. **İstatistikler:** Mesafe istatistiklerini gösterir
4. **CSV Export:** Ölçüm verilerini CSV dosyasına kaydeder
5. **Debug Modu:** Tüm debug mesajlarını gösterir veya filtreler

## Sorun Giderme

### Hiç ölçüm alınamıyor:
1. COM portunu kontrol et (`--list-ports` ile)
2. `COM_PORT` değişkenini doğru ayarla
3. Nucleo kartının çalıştığını kontrol et
4. STM32 kodunda "LiDAR Test Started" mesajı geliyor mu?

### Veri geliyor ama parse edilemiyor:
- Debug modunu aç (`DEBUG_MODE = True`)
- Gelen paket formatını kontrol et
- STM32 kodundaki `Process_LiDAR_Data` fonksiyonunu sensörün formatına göre güncelle

### Çok fazla debug mesajı:
- Normal moda geç (`DEBUG_MODE = False`)
- Sadece ölçüm verileri gösterilir
