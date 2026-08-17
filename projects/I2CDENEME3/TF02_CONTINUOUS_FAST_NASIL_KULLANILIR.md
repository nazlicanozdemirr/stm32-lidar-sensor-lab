# TF02-Pro Continuous Fast — Nasıl Kullanılır

Bu modül, **main.c içinde değil** ayrı dosyalarda duruyor; deneme için projeye ekleyip `main.c`'den çağıracaksın.

---

## 1. Dosyalar

| Dosya | Konum |
|-------|--------|
| `tf02_continuous_fast.h` | `Core/Inc/` |
| `tf02_continuous_fast.c` | `Core/Src/` |

Bu iki dosya projede zaten oluşturuldu.

---

## 2. Projeye kaynak dosyayı ekleme (STM32CubeIDE / Keil / Makefile)

- **STM32CubeIDE:** Project → sağ tık → Add Existing Sources to Project → `Core/Src/tf02_continuous_fast.c` seç.
- **Keil:** Project penceresinde gruba sağ tık → Add Existing Files → `tf02_continuous_fast.c` ekle.
- **Makefile / CMake:** `C_SOURCES` veya benzeri listeye `Core/Src/tf02_continuous_fast.c` ekle.

---

## 3. main.c değişiklikleri

`main.c` içinde:

1. **Include ekle** (USER CODE BEGIN Includes civarına):
   ```c
   #include "tf02_continuous_fast.h"
   ```

2. **Init sırasını** aşağıdaki gibi yap (I2C’den **önce** bus recovery):
   ```c
   MX_GPIO_Init();
   MX_USART2_UART_Init();   /* veya senin projede USART2 init adı ne ise */

   TF02_I2C1_BusRecover(); /* I2C init'ten ÖNCE */
   MX_I2C1_Init();

   TF02_Run_Continuous_Fast();  /* Bu fonksiyon içinde sonsuz döngü var, geri dönmez */
   ```

3. **Eski test çağrısını kaldır:** `Run_TFMini_I2C_MaxRate_Test_1s();` satırını sil veya yorum yap.

4. **while(1)** artık hiç çalışmayacak (program `TF02_Run_Continuous_Fast()` içinde dönecek). İstersen `while(1)` bloğunu boş bırakabilirsin.

---

## 4. Örnek main.c (sadece ilgili kısım)

```c
#include "main.h"
#include "tf02_continuous_fast.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
// ...

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_USART2_UART_Init();

  TF02_I2C1_BusRecover();
  MX_I2C1_Init();

  TF02_Run_Continuous_Fast();   /* sonsuz döngü burada */
  /* asla buraya gelmez */

  while (1) { }
}
```

---

## 5. Bu modülde ne var?

- **TF_GETCM_WAIT_MS = 2** (eskiden 5 ms) → daha yüksek Hz.
- GET_CM + 9 byte frame okuma, Median(5), değişimde (≥2 cm) UART’a yazdırma.
- Her 1 saniyede bir: OK/ERR sayısı, **Rate=… Hz**, Changes, LastRAW, LastMED.

Seri portta `Rate=... Hz` değerini izleyerek 2 ms ile frekans artışını görebilirsin. Hata artarsa `tf02_continuous_fast.c` içinde `TF_GETCM_WAIT_MS` değerini 3 veya 4 yapıp tekrar dene.
