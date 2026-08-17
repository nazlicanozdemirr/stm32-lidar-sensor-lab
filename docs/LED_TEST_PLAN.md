# LiDAR LED Test Planı – “Sensörden Veri Geliyor mu?”

Bu adım, tam Hz ölçümüne geçmeden önce **sadece** LiDAR’dan USART’a veri gelip gelmediğini Nucleo’daki **yeşil LED (LD2)** ile test etmek içindir.

## Amaç

- LiDAR besleme ve TX/RX kablolarının doğru olduğunu doğrulamak  
- USART1 (PA9/PA10) ayarının ve 115200 baud’un çalıştığını görmek  
- Sensör MODE pini 3.3V ise sürekli ölçüm paketleri göndermeli → LED yanıp söner  

## Donanım (aynı)

| LiDAR   | Nucleo      |
|--------|-------------|
| VCC    | 5V          |
| GND    | GND         |
| RX     | PA9 (USART1_TX) |
| TX     | PA10 (USART1_RX) |
| MODE   | 3.3V        |

USART2 / PC bağlantısı bu testte **gerekmez**.

## STM32CubeIDE Adımları

1. **Yeni proje** veya mevcut Nucleo projesi: MCU **STM32F446RE**, Board **Nucleo-64**.
2. **CubeMX / Pinout:**
   - **USART1:** Mode = Asynchronous, 115200, 8N1.  
     PA9 = USART1_TX, PA10 = USART1_RX (LiDAR’ın TX’i Nucleo’nun **RX = PA10**’a gelmeli).
   - **LD2:** Nucleo’da LD2 genelde PA5’e bağlı; PA5’i **GPIO_Output** yapın (zaten board ayarında olabilir).
   - **NVIC:** USART1 global interrupt **Enabled**.
3. **Code Generator:** HAL, default ayarlar.
4. **main.c ile tek dosyada yapmak (önerilen):**  
   Ekstra .c/.h gerekmez. `Core/Src/main_led_test_only.c` dosyasındaki tüm kodları **main.c** içine kopyalayın:
   - Başa: `s_lidar_*` değişkenleri, `lidar_led_test_*` fonksiyonları ve `HAL_UART_RxCpltCallback`.
   - `main()`: init’lerden sonra `lidar_led_test_init();`, döngüde `lidar_led_test_poll();`
5. *(İsteğe bağlı)* Ayrı modül kullanacaksanız projeye `lidar_led_test.c` ve `lidar_led_test.h` ekleyip main’de bunları çağırabilirsiniz.
6. Derleyip Nucleo’ya yükleyin.

## Beklenen Davranış

- **Veri geliyorsa:** LD2 (yeşil) sürekli yanıp söner (gelen her baytta toggle).  
  Sensör ~50 Hz ölçümde 9 bayt/frame → saniyede yüzlerce bayt → hızlı blink.
- **Veri gelmiyorsa:** Yaklaşık 500 ms sonra LED söner ve bir daha yanmaz (timeout).
- **Hiç yanmadıysa:** Besleme, GND, LiDAR TX → PA10 (RX) bağlantısı ve MODE=3.3V kontrol edin; USART1 115200 ve interrupt açık mı kontrol edin.

## Sonraki Adım

Bu testte LED düzgün yanıp söndüyse, aynı projede USART2’yi açıp `app_lidar` + `lidar_parser` modüllerine geçebilirsiniz; o zaman hem LED testini kaldırıp hem PC’ye `t_ms,distance_mm` log’u yollayıp Python ile Hz ölçümü yaparsınız.

## Özet Tablo

| Durum        | LED        | Anlamı                          |
|-------------|------------|----------------------------------|
| Hızlı blink | Veri var   | LiDAR + USART1 çalışıyor        |
| Sönük       | Timeout    | Veri yok (kablo/bağlantı/sensör)|
| Hep sönük   | Init sonrası hiç veri gelmedi | Bağlantı / MODE / baud kontrolü |
