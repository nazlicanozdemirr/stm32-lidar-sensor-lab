# SEN0585 LiDAR Sensör I2C Bağlantı Test Kılavuzu

## Test Özellikleri

Bu kod, SEN0585 LiDAR sensörünüzün STM32 Nucleo-F446RE ile I2C üzerinden doğru bağlanıp bağlanmadığını test eder.

### Test Edilen Özellikler:

1. **I2C Tarama**: Sensörün I2C bus'ında olup olmadığı kontrol edilir
2. **Adres Tespiti**: Sensörün I2C adresi otomatik olarak bulunur
3. **Veri Okuma**: Sensörden register okuma testi yapılır
4. **LED Göstergesi**: Bağlantı durumunu görsel olarak gösterir

## Bağlantı Kontrol Listesi

### SEN0585 LiDAR Sensör Bağlantıları:

1. **VCC** → Nucleo 3.3V veya 5V (sensörün çalışma voltajına göre)
2. **GND** → Nucleo GND
3. **SDA** → Nucleo PB7 (I2C1_SDA)
4. **SCL** → Nucleo PB6 (I2C1_SCL)

### Önemli Notlar:

- SEN0585 genellikle **I2C protokolü** ile çalışır
- I2C adresi genellikle **0x29** (VL53L0X/VL53L1X tabanlı sensörler için)
- Alternatif adresler: **0x52, 0x53, 0x54** (kod otomatik olarak tümünü tarar)
- I2C pull-up dirençleri genellikle sensör modülünde mevcuttur
- Eğer pull-up yoksa, 4.7kΩ pull-up dirençleri ekleyin

## LED Yapılandırması

Nucleo-F446RE'de LED pinleri:
- **LED2 (Yeşil)**: PA5 (LD2_Pin)

### LED Durumları:

- **Hızlı Yanıp Sönme (200ms)**: Sensör bulundu ve veri alınıyor ✅
- **Yavaş Yanıp Sönme (1000ms)**: Sensör bulunamadı veya bağlantı sorunu ❌

## CubeMX Yapılandırması

### I2C1 Ayarları:

1. **I2C1'i Aktif Edin:**
   - Mode: I2C
   - Configuration → Parameter Settings:
     - Speed: 100kHz (Standard Mode)
     - Duty Cycle: 2
     - Addressing Mode: 7-bit
     - General Call Address Detection: Disable

2. **I2C1 Pin Yapılandırması:**
   - PB6 → I2C1_SCL
   - PB7 → I2C1_SDA
   - Alternate Function: AF4_I2C1
   - GPIO Settings:
     - Mode: Alternate Function Open Drain
     - Pull-up: Pull-up
     - Speed: Very High

3. **UART2 (Debug için):**
   - PA2 → USART2_TX
   - PA3 → USART2_RX
   - Baud Rate: 115200

## Test Sonuçları

### Seri Port Çıktısı (UART2 - 115200 baud):

**Başarılı Bağlantı:**
```
========================================
STM32 SEN0585 LiDAR I2C Test
========================================
Scanning I2C bus for SEN0585 sensor...

[OK] Sensor found at I2C address: 0x29
[DATA] Register 0x00: 0xEE 0xAA
[OK] Sensor found at I2C address: 0x29
[DATA] Register 0x00: 0xEE 0xAA
```

**Bağlantı Sorunu:**
```
========================================
STM32 SEN0585 LiDAR I2C Test
========================================
Scanning I2C bus for SEN0585 sensor...

[SCAN] No sensor detected. Check I2C connections!
  - SDA: PB7 (I2C1) or PB9 (I2C1 alternate)
  - SCL: PB6 (I2C1) or PB8 (I2C1 alternate)
  - VCC: 3.3V or 5V
  - GND: GND
```

## Sorun Giderme

### 1. Hiç Sensör Bulunamıyor:

- [ ] Sensör güç bağlantısını kontrol edin (VCC ve GND)
- [ ] I2C kablolarını kontrol edin (SDA ve SCL)
- [ ] Pull-up dirençlerini kontrol edin (4.7kΩ, VCC'ye bağlı)
- [ ] CubeMX'te I2C1'in doğru pinlerde olduğunu kontrol edin (PB6, PB7)
- [ ] Sensörün çalışma voltajını kontrol edin (3.3V/5V)
- [ ] I2C bus'ında başka bir cihaz çakışması olup olmadığını kontrol edin

### 2. Sensör Bulunuyor Ama Veri Okunamıyor:

- [ ] I2C hızını kontrol edin (100kHz standart)
- [ ] Sensörün register haritasını kontrol edin (farklı register adresi gerekebilir)
- [ ] Sensörün başlatma (initialization) gerektirip gerektirmediğini kontrol edin
- [ ] Kablolarda gürültü olabilir (kısa kablolar kullanın, shield kullanın)

### 3. LED Yanmıyor:

- [ ] CubeMX'te LED pinini GPIO_Output olarak yapılandırdınız mı?
- [ ] LED pininin doğru olduğunu kontrol edin (PA5 - LD2)
- [ ] Kodun derlenip yüklendiğinden emin olun

### 4. I2C Timeout Hatası:

- [ ] Pull-up dirençlerini kontrol edin
- [ ] I2C hızını düşürmeyi deneyin (50kHz)
- [ ] Kabloların uzunluğunu kontrol edin (maksimum 30cm önerilir)
- [ ] Bus'ta başka bir cihazın çakışma yapıp yapmadığını kontrol edin

## Kod Özellikleri

### Otomatik Adres Tarama:

Kod şu I2C adreslerini otomatik olarak tarar:
- **0x29** (VL53L0X/VL53L1X için standart)
- **0x52** (Alternatif adres)
- **0x53** (Alternatif adres)
- **0x54** (Alternatif adres)

### Test Döngüsü:

- Her 2 saniyede bir sensör kontrol edilir
- Sensör bulunduğunda LED hızlı yanıp söner (200ms)
- Sensör bulunamadığında LED yavaş yanıp söner (1000ms)
- Seri port üzerinden detaylı bilgi gönderilir

## Sonraki Adımlar

Test başarılı olduktan sonra:

1. Sensörün gerçek register haritasını öğrenin
2. Mesafe okuma fonksiyonlarını ekleyin
3. Sensör başlatma (initialization) kodunu ekleyin
4. Kalibrasyon ayarlarını yapın
5. Normal çalışma moduna geçin

## Referanslar

- STM32F446RE Datasheet
- SEN0585 Product Page (DFRobot)
- VL53L0X/VL53L1X Datasheet (eğer bu sensörler tabanlıysa)
