# Sonraki Adım: Tam Projeye Geçiş

LED testi başarılı! Şimdi tam projeye geçelim: parser + PC'ye log + Python Hz ölçümü.

## CubeMX'te Yapılacaklar

1. **USART2 ekleyin** (henüz yoksa):
   - Mode: Asynchronous
   - Baud: 115200, 8N1
   - PA2 = USART2_TX, PA3 = USART2_RX (ST-Link VCP için)

2. **Code Generate** edin.

## main.c Güncellemesi

LED test kodunu kaldırıp **parser + PC log** kodunu ekleyin:

**Seçenek 1: Tek dosya (önerilen)**  
`Core/Src/main_full_lidar_single_file.c` dosyasındaki tüm kodu kendi main.c'nize kopyalayın. Bu dosya:
- Parser (CRC doğrulama, 9-byte frame parse)
- USART1 RX interrupt (LiDAR'dan bayt al)
- USART2 TX (PC'ye `t_ms,distance_mm\n` gönder)
- Hepsi tek main.c içinde, ekstra .c/.h yok

**Seçenek 2: Modüler (isteğe bağlı)**  
Eğer ayrı modüller isterseniz:
- `Core/Src/lidar_parser.c` + `Core/Inc/lidar_parser.h`
- `Core/Src/app_lidar.c` + `Core/Inc/app_lidar.h`
- main.c'de `#include "app_lidar.h"` ve `app_lidar_init()` + `app_lidar_poll()`

## Python Script Çalıştırma

1. Nucleo'yu USB ile PC'ye bağlayın (ST-Link VCP aktif olur).
2. Windows: Device Manager'da COM port numarasını bulun (örn. COM3).
3. Python script'i çalıştırın:

```bash
cd python
pip install pyserial
python lidar_rate_monitor.py COM3
```

4. Çıktı örneği:
```
current_rate_hz=48.2  avg_rate_hz=47.8  (count=1240)
```

## Beklenen Sonuç

- Python konsolunda sürekli güncellenen Hz değerleri görürsünüz.
- Sensör datasheet'e göre max ~50 Hz olmalı (mesafe/koşula göre değişebilir).
- `current_rate_hz`: Son 2 saniyedeki ortalama
- `avg_rate_hz`: Son 5 saniyedeki ortalama

## Sorun Giderme

- **Python "Port açılamadı" hatası:** COM port numarasını kontrol edin, başka program kullanıyor olabilir.
- **Hz = 0:** STM32'den PC'ye veri gelmiyor; USART2 bağlantısını ve baud'u kontrol edin.
- **Hz çok düşük:** LiDAR'dan az veri geliyor; MODE pini 3.3V'da mı, sensör önünde engel var mı kontrol edin.
