# STM32 Debug Kodu Kullanım Talimatı

## ÖNEMLİ: Bu kodları mevcut main.c dosyasına ekleme şekli

### Adım 1: STM32CubeIDE'de Projeyi Aç
- STM32CubeIDE'de projeni aç
- `Core/Src/main.c` dosyasını aç

### Adım 2: USER CODE Bölümlerini Bul
`main.c` dosyasında şu bölümleri bul:
- `/* USER CODE BEGIN Includes */`
- `/* USER CODE BEGIN PV */`
- `/* USER CODE BEGIN 0 */`
- `/* USER CODE BEGIN 2 */`
- `/* USER CODE BEGIN WHILE */`
- `/* USER CODE BEGIN 4 */`

### Adım 3: Kodları Kopyala-Yapıştır

`main_debug_complete.c` dosyasındaki kodları ilgili USER CODE bölümlerine ekle:

1. **USER CODE BEGIN Includes** → `#include <stdio.h>` ve `#include <string.h>` ekle
2. **USER CODE BEGIN PV** → Tüm değişken tanımlarını ekle
3. **USER CODE BEGIN 0** → `Process_LiDAR_Data` fonksiyonunu ekle
4. **USER CODE BEGIN 2** → UART başlatma kodlarını ekle
5. **USER CODE BEGIN WHILE** → İstatistik gönderme kodunu ekle
6. **USER CODE BEGIN 4** → `HAL_UART_RxCpltCallback` ve `HAL_UART_ErrorCallback` fonksiyonlarını ekle

### Adım 4: Derle ve Yükle
- Projeyi derle (Build)
- Nucleo kartına yükle (Run)

### Adım 5: Test Et
- Python scriptini çalıştır: `python lidar_speed_test.py`
- Veya debug scriptini çalıştır: `python debug_test.py`

## Beklenen Çıktı

### Başlangıçta göreceğin mesajlar:
```
========================================
LiDAR Debug Test Basladi
UART1 interrupt aktif ediliyor...
========================================

Nucleo hazir, sensorden veri bekleniyor...
```

### Sensörden veri geliyorsa:
```
RX[1]: 0x59 ('Y')
RX[2]: 0x59 ('Y')
RX[3]: 0x01 ('.')
...
PAKET[9]: 59 59 01 23 45 67 89 AB CD | ASCII: YY.#Eg...
MESAFE: 291 cm
```

### Her saniye istatistikler:
```
--- ISTATISTIKLER ---
Toplam paket: 125
Toplam byte: 1125
Olcum hizi: 125.00 Hz
----------------------
```

## Sorun Giderme

### Hiç veri gelmiyorsa:
1. Sensör bağlantılarını kontrol et
2. Sensörün LED'i yanıyor mu?
3. Baudrate ayarları doğru mu?
4. UART1 RX pin'i (PA10) doğru bağlı mı?

### Veri geliyor ama paket formatı yanlışsa:
- `Process_LiDAR_Data` fonksiyonunu sensörün gerçek paket formatına göre özelleştir
- Gelen paketleri analiz et ve formatı belirle

### Çok fazla debug mesajı geliyorsa:
- İlk 100 byte'dan sonra byte-byte gösterim durur
- Sadece paketler gösterilmeye devam eder
