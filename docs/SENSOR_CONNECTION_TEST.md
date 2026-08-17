# SEN0885 LiDAR Sensör Bağlantı Test Kılavuzu

## Test Özellikleri

Bu kod, SEN0885 LiDAR sensörünüzün STM32 Nucleo-F446RE ile doğru bağlanıp bağlanmadığını test eder.

### Test Edilen Özellikler:

1. **Veri Alımı**: Sensörden UART üzerinden veri gelip gelmediği kontrol edilir
2. **Paket Doğrulama**: Gelen verilerin doğru formatta olup olmadığı kontrol edilir
3. **Bağlantı Durumu**: Sensör bağlı mı değil mi otomatik tespit edilir
4. **LED Göstergesi**: Bağlantı durumunu görsel olarak gösterir

## Bağlantı Kontrol Listesi

### SEN0885 LiDAR Sensör Bağlantıları:

1. **VCC** → Nucleo 5V veya 3.3V (sensörün çalışma voltajına göre)
2. **GND** → Nucleo GND
3. **TX** → Nucleo UART1 RX (PA10 veya PA3 - CubeMX'te ayarladığınız pini kontrol edin)
4. **RX** → Nucleo UART1 TX (PA9 veya PA2 - CubeMX'te ayarladığınız pini kontrol edin)

### Önemli Notlar:

- SEN0885 genellikle **115200 baud rate** ile çalışır
- Sensör **5V** veya **3.3V** ile çalışabilir (datasheet'e bakın)
- UART seviyeleri doğru olmalı (3.3V logic için level shifter gerekebilir)

## LED Yapılandırması

Nucleo-F446RE'de LED pinleri:
- **LED2 (Yeşil)**: PA5
- **LED3 (Kırmızı)**: PB14 veya PB0

### LED'i Aktif Etmek İçin:

1. **CubeMX'te LED Pinini Yapılandırın:**
   - PA5'i GPIO_Output olarak ayarlayın
   - Pin adını "LED2" olarak isimlendirin

2. **Kodda LED Pinini Aktif Edin:**
   - `main.c` dosyasında `UpdateLEDStatus()` fonksiyonunu bulun
   - TODO yorumunu kaldırın ve LED pininizi ekleyin:
   
   ```c
   // LED2 için (PA5):
   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, ledState ? GPIO_PIN_SET : GPIO_PIN_RESET);
   
   // Veya LED3 için (PB14):
   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, ledState ? GPIO_PIN_SET : GPIO_PIN_RESET);
   ```

## Test Sonuçları

### Seri Port Çıktısı (UART2):

**Başarılı Bağlantı:**
```
========================================
STM32 SEN0885 LiDAR Connection Test
========================================
Waiting for sensor data...
>>> SENSOR CONNECTED! <<<
RAW[59 59 00 05 12 34 56 78 AB CD EF 12 34 56 78 9A BC DE]
[OK] Header found at index: 0
[VALID] Distance:1280cm Strength:13398 Temp:22136
```

**Bağlantı Sorunu:**
```
[WARNING] No header found - Check baud rate!
[STATUS] No sensor data for 5000 ms - Check connections!
```

### LED Durumları:

- **Hızlı Yanıp Sönme (500ms)**: Sensör bağlı ve veri alınıyor ✅
- **Yavaş Yanıp Sönme**: Sensör bağlı değil veya veri gelmiyor ❌
- **Sürekli Yanık**: Kod hatası (kontrol edin)

## Sorun Giderme

### 1. Hiç Veri Gelmiyor:

- [ ] Sensör güç bağlantısını kontrol edin (VCC ve GND)
- [ ] UART kablolarını kontrol edin (TX-RX çapraz bağlı olmalı)
- [ ] Baud rate'i kontrol edin (115200 olmalı)
- [ ] CubeMX'te UART1'in doğru pinlerde olduğunu kontrol edin
- [ ] Sensörün çalışma voltajını kontrol edin (5V/3.3V)

### 2. Veri Geliyor Ama Header Bulunamıyor:

- [ ] Baud rate yanlış olabilir (115200 kontrol edin)
- [ ] UART pinleri yanlış bağlı olabilir
- [ ] Sensör farklı bir protokol kullanıyor olabilir

### 3. Header Bulunuyor Ama CRC Hatalı:

- [ ] Kablolarda gürültü olabilir (kısa kablolar kullanın)
- [ ] Baud rate tam olarak doğru olmayabilir
- [ ] Sensör farklı bir CRC algoritması kullanıyor olabilir

### 4. LED Yanmıyor:

- [ ] CubeMX'te LED pinini GPIO_Output olarak yapılandırdınız mı?
- [ ] `UpdateLEDStatus()` fonksiyonundaki TODO yorumunu kaldırdınız mı?
- [ ] Doğru GPIO pinini kullanıyor musunuz?

## Test Modu

Kod şu anda **TEST_MODE = 1** olarak ayarlanmış. Bu modda:
- Ham veri hex formatında gösterilir
- Detaylı debug mesajları gönderilir
- Bağlantı durumu sürekli kontrol edilir

Normal kullanım için `#define TEST_MODE 0` yapın.

## Sonraki Adımlar

Test başarılı olduktan sonra:
1. `TEST_MODE`'u 0 yapın
2. LED pinini aktif edin
3. Normal çalışma moduna geçin
4. Mesafe verilerini kullanmaya başlayın
