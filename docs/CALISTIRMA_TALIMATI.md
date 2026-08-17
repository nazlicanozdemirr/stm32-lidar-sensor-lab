# Python Scriptini Çalıştırma Talimatı

## Doğru Komut

PowerShell'de şu komutu kullan:

```bash
python lidar_speed_test_debug.py
```

## Adım Adım

1. **PowerShell'i aç** (zaten açık görünüyor)

2. **Scriptin bulunduğu klasöre git:**
   ```bash
   cd C:\Users\PC_2255
   ```
   (veya scriptin bulunduğu klasöre)

3. **Scripti çalıştır:**
   ```bash
   python lidar_speed_test_debug.py
   ```

## Alternatif Komutlar

Eğer `python` komutu çalışmazsa:

```bash
python3 lidar_speed_test_debug.py
```

veya

```bash
py lidar_speed_test_debug.py
```

## Beklenen Çıktı

Script çalıştığında şunları görmelisin:

```
======================================================================
LiDAR Sensör Ölçüm Hızı Testi - Debug Modu
======================================================================
Port: COM10
Baudrate: 115200
Test süresi: 10 saniye
Debug modu: Açık

✓ COM10 portu 115200 bps ile açıldı.
✓ 10 saniye boyunca ölçüm yapılıyor...
```

## Sorun Giderme

### "python komutu bulunamadı" hatası alırsan:

1. Python'un yüklü olduğunu kontrol et:
   ```bash
   python --version
   ```

2. Eğer yüklü değilse, Python'u yükle: https://www.python.org/downloads/

3. Eğer yüklüyse ama komut çalışmıyorsa, tam yol ile çalıştır:
   ```bash
   C:\Python3x\python.exe lidar_speed_test_debug.py
   ```

### Script bulunamadı hatası alırsan:

Scriptin bulunduğu klasöre git:
```bash
cd "C:\Users\PC_2255"
```

Veya scriptin tam yolunu kullan:
```bash
python "C:\Users\PC_2255\lidar_speed_test_debug.py"
```
