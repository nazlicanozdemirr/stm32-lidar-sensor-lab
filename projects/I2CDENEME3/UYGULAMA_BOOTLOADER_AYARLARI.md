# Uygulama (I2CDENEME3) – Bootloader ile Kullanım Ayarları

Bootloader uygulama bölgesini **0x08008000** kabul ediyor. Bu projeyi “uygulama” olarak kullanacaksanız aşağıdakileri yapın.

---

## 1. Linker script: Flash başlangıcını değiştir

**Dosya:** `STM32F446RETX_FLASH.ld`

**Değişiklik:** `MEMORY` bölümünde `FLASH` satırını şu şekilde güncelleyin:

```
FLASH (rx) : ORIGIN = 0x08008000, LENGTH = 480K
```

- **ORIGIN = 0x08008000** → Bootloader’ın bıraktığı adres (32 KB sonrası).
- **LENGTH = 480K** → 512K - 32K (bootloader alanı).

---

## 2. Vektör tablosu ofseti (VTOR)

Uygulama kendi vektör tablosunu **0x08008000**’de kullanmalı. `main.c` içinde `main()` başında (veya `SystemClock_Config()` çağrısından hemen sonra) ekleyin:

```c
/* Bootloader ile kullanım: vektör tablosu uygulama adresinde */
SCB->VTOR = 0x08008000;
```

`#include "stm32f4xx.h"` veya `"main.h"` zaten varsa `SCB` kullanılabilir.

---

## 3. (İsteğe bağlı) Magic imza

`bootloader.h` içinde `BOOTLOADER_USE_MAGIC` 1 ise, uygulama **0x08008200** adresine `0x55424F4F` değerini yazmalı. Bu genelde linker’da bir section ile yapılır:

- Linker’a `.app_magic` section ekleyip bu adrese sabit değer yerleştirilebilir,  
- veya `main()` başında bir kez bu adrese yazılabilir (o adres uygulama flash’ında olduğu için sadece “imza var” anlamında kullanılır; her açılışta zaten orada kalır).

Magic kullanmak istemiyorsanız bootloader tarafında `BOOTLOADER_USE_MAGIC` değerini **0** bırakın (varsayılan örnekte 0).

---

## 4. Derleme ve yükleme sırası

1. **Bootloader** projesini derleyip **0x08000000**’e yükleyin (ilk 32 KB).
2. **Bu uygulama** (I2CDENEME3) projesini yukarıdaki linker/VTOR ayarlarıyla derleyin; çıkan `.bin` dosyasını **0x08008000**’e yükleyin (ST-Link Utility, OpenOCD veya bootloader’ın UART yükleme özelliği ile).

Bu sıra ile bootloader açılışta “firmware var mı?” diye bakar; varsa doğrudan bu uygulamaya atlar.
