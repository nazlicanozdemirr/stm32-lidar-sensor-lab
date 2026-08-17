# Python Script Kullanım Kılavuzu

## 1. Gereksinimler

```bash
pip install pyserial
```

veya

```bash
pip install -r requirements.txt
```

## 2. COM Port Numarasını Bulma

### Windows:
1. **Device Manager** açın (Win+X → Device Manager)
2. **Ports (COM & LPT)** altında **"STMicroelectronics STLink Virtual COM Port (COMx)"** görünür
3. COM numarasını not edin (örn. COM3, COM4, COM5)

### Alternatif (PowerShell):
```powershell
Get-PnPDevice -Class Ports | Where-Object {$_.FriendlyName -like "*STLink*"}
```

## 3. Script'i Çalıştırma

### Basit kullanım (varsayılan COM3):
```bash
python lidar_rate_monitor.py COM3
```

### Farklı COM port:
```bash
python lidar_rate_monitor.py COM5
```

### Parametreler:
```bash
# Baud rate değiştirme (varsayılan 115200)
python lidar_rate_monitor.py COM3 -b 115200

# Pencere sürelerini değiştirme
python lidar_rate_monitor.py COM3 -f 10 -s 3
# -f: Sabit pencere (varsayılan 5 saniye)
# -s: Kayar pencere (varsayılan 2 saniye)
```

## 4. Beklenen Çıktı

```
Port: COM3 @ 115200. Çıkmak için Ctrl+C.
Format: current_rate_hz (kayar pencere) | avg_rate_hz (sabit pencere)
------------------------------------------------------------
current_rate_hz=0.0  avg_rate_hz=0.0  (count=0)
current_rate_hz=12.5  avg_rate_hz=10.2  (count=51)
current_rate_hz=48.3  avg_rate_hz=45.1  (count=226)
current_rate_hz=49.2  avg_rate_hz=47.8  (count=1240)
```

- **current_rate_hz**: Son 2 saniyedeki ölçüm sayısı / 2 (anlık hız)
- **avg_rate_hz**: Son 5 saniyedeki ölçüm sayısı / 5 (ortalama hız)
- **count**: Başlangıçtan beri toplam ölçüm sayısı

## 5. Sorun Giderme

### "Port açılamadı" hatası:
- COM port numarasını kontrol edin
- Başka bir program (Serial Monitor, PuTTY vb.) portu kullanıyor olabilir
- Nucleo'nun USB kablosu bağlı mı kontrol edin
- Device Manager'da port görünüyor mu kontrol edin

### Hz = 0.0 görünüyorsa:
- STM32'den PC'ye veri gelmiyor
- USART2 (PA2/PA3) bağlantısını kontrol edin
- STM32 kodunda `lidar_app_init()` çağrıldı mı?
- STM32'de USART2 init edildi mi?
- Baud rate eşleşiyor mu? (115200)

### Hz çok düşükse:
- LiDAR'dan az veri geliyor olabilir
- MODE pini 3.3V'da mı? (sürekli ölçüm için)
- Sensör önünde engel var mı?
- LiDAR sensörü çalışıyor mu? (LED testi yapıldı mı?)

### Veri geliyor ama parse edilemiyorsa:
- STM32'den gelen format `t_ms,distance_mm\n` olmalı
- Serial Monitor ile ham veriyi kontrol edin (PuTTY, Arduino Serial Monitor vb.)

## 6. Çıkış

Script'i durdurmak için: **Ctrl+C**
