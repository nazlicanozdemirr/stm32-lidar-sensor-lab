# Bootloader Tasarımı – Adım Adım

## Amaç
Mikroişlemciye yüklenecek **uygulama firmware’inin flash’ta zaten mevcut olup olmadığını** kontrol eden;  
- **Varsa** → yükleme işlemini atlayıp doğrudan uygulamaya geçen,  
- **Yoksa** → yükleme sürecini (UART ile veri alıp flash’a yazma) başlatan bir bootloader.

---

## Adım 1: Bellek (Flash) Haritası

**STM32F446RET6:** 512 KB Flash, 128 KB RAM.

| Bölge        | Başlangıç Adresi | Boyut   | Açıklama                          |
|-------------|-------------------|---------|-----------------------------------|
| Bootloader  | 0x0800 0000       | 32 KB   | Bu bootloader kodu                |
| Uygulama    | 0x0800 8000       | 480 KB  | Ana uygulama (I2CDENEME3 vb.)     |

- Bootloader her açılışta çalışır.
- Uygulama, **0x08008000** adresinden başlayacak şekilde derlenmeli (linker ve vektör tablosu ofseti).

---

## Adım 2: “Dosya (Firmware) Var mı?” Kriteri

Uygulama bölgesinde **geçerli firmware** sayılması için:

1. **Stack Pointer (SP)** – Uygulama vektör tablosunun ilk kelimesi (0x08008000) **0xFFFFFFFF olmamalı** (boş flash).
2. **Reset Handler** – İkinci kelime (0x08008004) **0xFFFFFFFF olmamalı**.
3. **İsteğe bağlı: Magic imza** – Belirli bir ofsette (örn. 0x08008200) sabit bir değer (örn. `APP_MAGIC = 0x55424F4F`) okunur; eşleşirse “imzalı uygulama” kabul edilir.

Özet:  
- İlk iki kelime boş (0xFFFFFFFF) ise → **firmware yok** → yükleme moduna gir.  
- Geçerli SP + Reset adresi (ve istenirse magic) varsa → **firmware var** → yükleme atlanır, uygulamaya zıplanır.

---

## Adım 3: Bootloader Akış Şeması (Algoritma)

```
  [ Açılış ]
       |
       v
  Donanım / RCC / GPIO / UART (minimal) init
       |
       v
  Uygulama bölgesini kontrol et (0x08008000):
  - İlk kelime == 0xFFFFFFFF ?  --> EVET --> [ Yükleme modu ]
  - İkinci kelime == 0xFFFFFFFF ? --> EVET --> [ Yükleme modu ]
  - İsteğe bağlı: Magic imza doğru mu? --> HAYIR --> [ Yükleme modu ]
       |
       v HAYIR (yani geçerli firmware var)
  [ Uygulama var: yükleme atla ]
  - MSP ve Reset_Handler'ı uygulama adresinden oku
  - VECT_TAB ofsetini 0x8000 yap (SysTick/NVIC için)
  - Uygulama stack'ine ve Reset_Handler'a jump
       |
       v EVET (firmware yok veya geçersiz)
  [ Yükleme modu ]
  - UART üzerinden komut bekleyecek şekilde döngüye gir
  - Gelen .bin verisini 0x08008000’den itibaren flash’a yaz
  - Yükleme bitince (ve istenirse doğrulama sonrası) uygulamaya zıpla veya reset at
```

---

## Adım 4: Uygulama Tarafı (I2CDENEME3) Yapılacaklar

Bootloader ile birlikte kullanmak için **uygulama projesinde** yapılacaklar **UYGULAMA_BOOTLOADER_AYARLARI.md** dosyasında adım adım anlatıldı. Özet:

1. **Linker script (STM32F446RETX_FLASH.ld)**  
   - `FLASH` memory’de `ORIGIN = 0x08008000` ve `LENGTH = 480K` (veya 512K - 32K) yapılmalı.

2. **Vektör tablosu ofseti**  
   - `main.c` veya `SystemInit` sonrası:  
     `SCB->VTOR = 0x08008000;`

3. **Derleme çıktısı**  
   - Uygulama `.bin` dosyası 0x08008000’e göre üretilmeli (linker değişince otomatik olur).

Bu adımlar yapılmazsa bootloader “uygulama var” deyip zıplasa bile uygulama yanlış adreste çalışır veya çökebilir.

---

## Adım 5: Yükleme Protokolü (Özet)

- **Arayüz:** UART (örn. USART2, mevcut projenizle uyumlu).
- Bootloader yükleme modunda iken:
  - Host (PC) belirli bir komut (örn. `0x01`) gönderir.
  - Ardından boyut (4 byte) ve ardından .bin içeriği gönderilir.
  - Bootloader gelen veriyi 0x08008000’den itibaren flash’a yazar (sayfa/sector erase + program).
- İsteğe bağlı: Sonunda checksum ile doğrulama, sonra uygulamaya zıplama veya NVIC_SystemReset.

---

## Özet Tablo

| Durum              | Bootloader davranışı                    |
|--------------------|-----------------------------------------|
| Firmware **var**   | Yükleme atlanır → doğrudan uygulamaya geçilir. |
| Firmware **yok**   | Yükleme süreci başlatılır (UART’tan veri alınıp flash’a yazılır). |

---

## Bootloader projesinde main() kullanımı

Bootloader’ı ayrı bir proje olarak **0x08000000**’e derleyip yükleyecekseniz, `main()` içinde minimal init (RCC, GPIO, isteğe bağlı UART) sonrası şunu çağırın:

```c
#include "bootloader.h"

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  /* GPIO/UART vb. minimal init */
  Bootloader_CheckAndRun();
  return 0;  /* Sadece yükleme modundan çıkışla buraya düşülebilir */
}
```

Kodlar `Core/Inc/bootloader.h` ve `Core/Src/bootloader.c` olarak eklendi. Bootloader projesinin linker’ında FLASH ORIGIN = 0x08000000, LENGTH = 32K kullanın.
