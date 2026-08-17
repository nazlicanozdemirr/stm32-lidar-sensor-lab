# STM32 Debug Kodları

## Sorun: Hiç ölçüm alınamıyor

### Adım 1: Nucleo'nun çalıştığını kontrol et

`main.c` dosyasında **USER CODE BEGIN 2** bölümüne şunu ekle:

```c
// Test mesajı - her saniye gönder
const char *test_msg = "Nucleo Calisiyor\r\n";
HAL_UART_Transmit(&huart2, (uint8_t*)test_msg, strlen(test_msg), HAL_MAX_DELAY);
```

Bu mesajı Python scriptinde görüyorsan, Nucleo çalışıyor demektir.

### Adım 2: UART1'den gelen ham veriyi göster

`HAL_UART_RxCpltCallback` fonksiyonunu şu şekilde güncelle:

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    uint8_t received_byte = uart1_rx_buffer[uart1_rx_index];
    uart1_rx_index++;
    
    // DEBUG: Her gelen byte'ı PC'ye gönder
    char debug_msg[64];
    sprintf(debug_msg, "RX[%d]: 0x%02X\r\n", uart1_rx_index, received_byte);
    HAL_UART_Transmit(&huart2, (uint8_t*)debug_msg, strlen(debug_msg), HAL_MAX_DELAY);
    
    // Eğer buffer dolduysa veya paket tamamlandıysa işle
    if (received_byte == '\n' || uart1_rx_index >= UART1_RX_BUFFER_SIZE - 1)
    {
      // Veriyi işle
      Process_LiDAR_Data(uart1_rx_buffer, uart1_rx_index);
      
      // Buffer'ı temizle
      uart1_rx_index = 0;
      memset(uart1_rx_buffer, 0, UART1_RX_BUFFER_SIZE);
    }
    
    // Sonraki byte'ı dinlemeye devam et
    HAL_UART_Receive_IT(&huart1, &uart1_rx_buffer[uart1_rx_index], 1);
  }
}
```

Bu kod her gelen byte'ı PC'ye gönderir. Böylece sensörden veri gelip gelmediğini görebilirsin.

### Adım 3: Sensörün paket formatını kontrol et

Eğer sensörden veri geliyorsa ama `Process_LiDAR_Data` çalışmıyorsa, paket formatı yanlış olabilir.

`Process_LiDAR_Data` fonksiyonunu şu şekilde güncelle (tüm paketleri kabul et):

```c
void Process_LiDAR_Data(uint8_t *data, uint16_t length)
{
  // DEBUG: Gelen paketi göster
  char debug_packet[128];
  sprintf(debug_packet, "PAKET[%d]: ", length);
  HAL_UART_Transmit(&huart2, (uint8_t*)debug_packet, strlen(debug_packet), HAL_MAX_DELAY);
  
  for (int i = 0; i < length && i < 16; i++)
  {
    char hex[8];
    sprintf(hex, "%02X ", data[i]);
    HAL_UART_Transmit(&huart2, (uint8_t*)hex, strlen(hex), HAL_MAX_DELAY);
  }
  HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
  
  // Ölçüm sayacını artır (her paket için)
  measurement_count++;
  
  // Mesafe değerini çıkar (sensörün formatına göre özelleştir)
  // ÖRNEK: İlk 2 byte mesafe ise:
  if (length >= 2)
  {
    lidar_distance = (uint16_t)(data[0] | (data[1] << 8));
    
    char output[64];
    sprintf(output, "%lu,%u\r\n", HAL_GetTick(), lidar_distance);
    HAL_UART_Transmit(&huart2, (uint8_t*)output, strlen(output), HAL_MAX_DELAY);
  }
}
```

### Adım 4: Sensör bağlantılarını kontrol et

1. **Sensörün TX çıkışı** → Nucleo'nun **UART1 RX (PA10)** pinine bağlı mı?
2. **GND** → GND'ye bağlı mı?
3. **VCC** → Doğru voltaja bağlı mı? (3.3V veya 5V)

### Adım 5: Baudrate kontrolü

STM32CubeIDE'de UART1'in baudrate'i sensörünüzün baudrate'i ile eşleşmeli. Sensörün datasheet'ine bak.

### Adım 6: Sensörün çalıştığını kontrol et

- Sensörün LED'i yanıyor mu?
- Sensörün kendi ekranında değerler görünüyor mu?
- Sensörün datasheet'ine bakarak test komutları var mı?
