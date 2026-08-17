# USART2 Testi – PC'ye Veri Gidiyor mu?

Count=0 ise önce **sadece USART2** ile test edin: STM32 her 1 saniyede bir "test" satırı göndersin. Python'da veya `raw_serial_check.py` ile bu satırı görüyorsanız donanım/yol doğrudur.

## 1. main.c içinde geçici olarak bunu kullanın

LiDAR/parser kodunu **geçici olarak** kapatıp aşağıdaki minimal kodu çalıştırın.

**CubeMX:** USART2 açık (PA2=TX, PA3=RX), 115200 8N1. Başka bir şey gerekmez (USART1 kapalı bile olabilir).

```c
#include "main.h"
#include "usart.h"
#include "gpio.h"

extern UART_HandleTypeDef huart2;

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  const char *msg = "test\r\n";
  uint32_t last = HAL_GetTick();

  while (1)
  {
    uint32_t now = HAL_GetTick();
    if (now - last >= 1000) {
      last = now;
      HAL_UART_Transmit(&huart2, (const uint8_t *)msg, (uint16_t)6, 100);
    }
  }
}
```

- Derleyip yükleyin.
- `raw_serial_check.py` ile aynı COM portu açın:  
  `python raw_serial_check.py COM10`
- **Her saniye "test" yazıyorsa** → USART2 + ST-Link VCP + port doğru. Sorun parser/LiDAR tarafındadır; tam koda dönüp `lidar_app_init` ve USART1’i kontrol edin.
- **Hiçbir şey gelmiyorsa** → Yanlış COM port (ST-Link’in **VCP** portunu kullanın, sadece debug portu değil) veya Nucleo’da USART2’nin ST-Link’e bağlı olup olmadığını (SB13/SB14/SB62/SB63) kontrol edin.

## 2. Doğru COM portu

Nucleo takılıyken Device Manager’da genelde **iki** port görünür:

- **STMicroelectronics STLink Virtual COM Port (COMx)** → Bunu kullanın (USART2 buraya gider).
- Diğeri sadece debug için olabilir; veri oraya gitmez.

`python lidar_rate_monitor.py -l` ile portları listeleyip **Virtual COM Port** yazanı seçin.

## 3. Test başarılıysa

"test" satırları geliyorsa:

1. Bu minimal kodu kaldırıp **tam LiDAR koduna** (parser + USART1 + USART2) geri dönün.
2. main’de `MX_USART1_UART_Init();` ve `lidar_app_init();` çağrıldığından emin olun.
3. `while(1)` içinde `lidar_app_poll();` olsun.
4. Tekrar aynı COM portla `lidar_rate_monitor.py` veya `raw_serial_check.py` çalıştırın.

Hâlâ count=0 ise LiDAR’dan geçerli çerçeve gelmiyordur (bağlantı, baud, MODE pini vb. kontrol edin).
