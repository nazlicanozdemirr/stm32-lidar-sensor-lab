# Bootloader — Linker ve STM32CubeIDE .ioc Ayarları

## 1. Bootloader projesi (Sector 0)

- **Linker script:** `STM32F446RETX_BOOT.ld`
- **ROM:** `ORIGIN = 0x08000000`, `LENGTH = 16K` (Sector 0)
- **Main:** Bootloader build'de `main_bootloader.c` kullanılmalı (veya `main.c` içinde sadece `Bootloader_CheckAndRun()` çağrılmalı).

### main_bootloader.c örneği (zaten mevcut)

```c
int main(void)
{
  HAL_Init();
  SystemClock_Config_Boot();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  Bootloader_CheckAndRun();

  return 0; /* Buraya düşmez */
}
```

---

## 2. Uygulama projesi (Sector 2–7)

Uygulama firmware’i **0x08008000** adresinde başlamalı (Sector 2).

- **Linker script (uygulama):** Ayrı bir script kullanın, örnek isim: `STM32F446RETX_APP.ld`
- **ROM:** `ORIGIN = 0x08008000`, `LENGTH = 480K` (0x78000 — Sector 2–7)
- **Vector table:** Bootloader atlarken `SCB->VTOR = 0x08008000` yaptığı için uygulama tarafında ekstra VTOR ayarı gerekmez.

### STM32CubeIDE’de uygulama linker ayarı

1. Proje → Properties → C/C++ Build → Settings → MCU GCC Linker → General
2. “Script file” ile uygulama için özel `.ld` dosyasını seçin (örn. `STM32F446RETX_APP.ld`).

### Örnek uygulama linker (MEMORY kısmı)

```ld
MEMORY
{
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
  FLASH (rx)  : ORIGIN = 0x08008000, LENGTH = 480K
}
```

Kalan bölümler (SECTIONS vb.) standart STM32 script ile aynı kalabilir; sadece `ORIGIN`/`LENGTH` farklıdır.

---

## 3. STM32CubeIDE .ioc — UART2

- **USART2:** Etkin
- **Mode:** Asynchronous
- **Baud Rate:** 115200
- **Word Length:** 8 Bits
- **Stop Bits:** 1
- **Parity:** None
- **Pinler (Nucleo F446RE):**
  - **USART2_TX:** PA2 (Alternate: AF7)
  - **USART2_RX:** PA3 (Alternate: AF7)

CubeMX’te: **Connectivity → USART2** → yukarıdaki değerleri seçin; pinler otomatik atanır.

---

## 4. Özet

| Bileşen        | Adres / Değer        |
|----------------|----------------------|
| Bootloader     | 0x08000000, 16 KB    |
| Metadata       | 0x08004000, Sector 1  |
| Uygulama       | 0x08008000, 480 KB   |
| UART2          | 115200, 8N1, PA2/PA3  |
