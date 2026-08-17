# Bootloader Nasıl Test Edilir?

## Gerekenler

- **Donanım:** NUCLEO-F446RE (veya STM32 + USART2). USART2: PA2 (TX), PA3 (RX), **115200 baud**.
- **PC:** Python 3, `pyserial` (`pip install pyserial`).
- **İki firmware:**  
  1) **Bootloader** → 0x08000000 (ilk 32 KB),  
  2) **Uygulama** (I2CDENEME3) → 0x08008000 (480 KB).

---

## Adım 1: Bootloader firmware’ini hazırla

Bootloader’ın açılışta çalışması için **ayrı bir “bootloader” build** gerekir:

- **Linker:** FLASH `ORIGIN = 0x08000000`, `LENGTH = 32K`.
- **main()** sadece şunu yapsın:
  - `HAL_Init()`, `SystemClock_Config()`, GPIO, **MX_USART2_UART_Init()**
  - Sonra **`Bootloader_CheckAndRun()`** → `return 0;`’a hiç düşmez (ya uygulamaya zıplar ya da yükleme modunda kalır).

Bu projede bunu yapmanın pratik yolu:

- Yeni bir **Build Configuration** (örn. “Bootloader”) ekleyin.
- Bu konfigürasyonda:
  - Linker script olarak **sadece bootloader için** `ORIGIN = 0x08000000`, `LENGTH = 32K` olan bir `.ld` kullanın.
  - `main.c` yerine **sadece bootloader main’i** derlenecek bir kaynak kullanın (örn. `main_bootloader.c`).

**Örnek `main_bootloader.c`** (projeye ekleyip sadece Bootloader build’de derletin):

```c
#include "main.h"
#include "bootloader.h"
#include "gpio.h"
#include "usart.h"

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  Bootloader_CheckAndRun();
  return 0;
}
```

Bootloader build’i derleyip çıkan **.bin**’i 0x08000000’e yükleyin (ST-Link Utility veya STM32CubeProgrammer).

---

## Adım 2: Uygulama (I2CDENEME3) build’i

- **Linker:** `ORIGIN = 0x08008000`, `LENGTH = 480K` (UYGULAMA_BOOTLOADER_AYARLARI.md’deki gibi).
- **main.c** başında: `SCB->VTOR = 0x08008000;`
- Normal **Debug/Release** build’i alın; çıkan **.bin** veya **.hex** uygulama firmware’i olacak.

Bu .bin’i **0x08008000’e elle yüklemeyin**; testte Python script ile bootloader’a yükleyeceksiniz.

---

## Adım 3: Cihazı “yükleme moduna” getir

- Bootloader’ı 0x08000000’e yükledikten sonra **uygulama bölgesini boş bırakın** (veya 0x08008000’den itibaren sektörleri silin).
- **Reset** atın → Bootloader çalışır, uygulama yoksa **yükleme moduna** girer (UART’tan komut bekler).

(İsteğe bağlı: STM32CubeProgrammer ile “Full chip erase” yapıp sonra sadece 0x08000000’e bootloader .bin’i yükleyin; böylece 0x08008000 boş kalır.)

---

## Adım 4: Python ile firmware yükle

1. NUCLEO’nun **ST-Link USB** veya **UART (PA2/PA3)** bağlantısını kullanın.  
   Sanal COM port (örn. COM3) cihaz yöneticisinde görünmeli.

2. Komut satırında:

```bash
cd C:\Users\PC_2255\Desktop\deneme\I2CDENEME3\tools
python upload_firmware.py -p COM3 ..\Debug\I2CDENEME3.bin
```

(COM port ve .bin yolunu kendi build çıktınıza göre değiştirin.)

3. Beklenen: “Yükleme başlıyor...” ve “Yükleme tamamlandı.”  
   Sonra cihaz uygulamaya zıplar (TF-Mini test vb. çalışır).

---

## Adım 5: “Zaten yüklü” testi (hash kontrolü)

**Aynı** .bin dosyasıyla komutu **tekrar** çalıştırın:

```bash
python upload_firmware.py -p COM3 ..\Debug\I2CDENEME3.bin
```

- Önce cihazı **reset**leyin (bootloader tekrar çalışsın; uygulama varsa yine bootloader önce kontrol eder, sonra uygulamaya gider – ama bootloader’a dönmek için reset gerekir).  
  Aslında reset sonrası bootloader “firmware var” deyip doğrudan uygulamaya gideceği için, **ikinci test** için:
  - Ya cihazı **bootloader modunda** bırakacak bir “bootloader’da kal” seçeneği kullanırsınız,
  - Ya da **uygulama yüklüyken** script’i çalıştırırsınız; bu durumda cihaz uygulama modunda olduğu için UART’ta bootloader komutu dinlemez, script timeout alır.

Doğru “zaten yüklü” senaryosu:

- **Yöntem A:** Bootloader’a “komut bekle” modunda kalma özelliği ekleyin (örn. belirli bir pin HIGH iken açılışta yükleme modunda kal). Böylece reset → bootloader yükleme modunda kalır → aynı .bin ile script’i çalıştırın → **“Firmware zaten yüklü, yükleme atlanıyor.”** mesajını görürsünüz.
- **Yöntem B:** İki kez yükleme: İlk seferde .bin’i yükleyin (cihaz uygulamaya gider). İkinci test için uygulama bölgesini **silmeden** cihazı resetleyin; bootloader yine “firmware var” deyip uygulamaya atlar. Bu durumda “zaten yüklü” mesajını görmek için script’in **cihaz bootloader’dayken** çalışması gerekir.  
  En basit yol: **Bootloader’da kısa bir “pencerede” (örn. açılışta 3 saniye) UART’ı dinle.** Bu 3 saniyede script 0x02 (hash kontrol) gönderirse cevap döner. Şu anki kodda bootloader firmware yoksa süresiz bekliyor; firmware varsa doğrudan uygulamaya atlıyor.  
  Bu yüzden “zaten yüklü” testi için:
  - Ya bootloader’a “force update mode” (örn. bir pin veya UART’ta özel komut) eklersiniz,
  - Ya da **ilk yüklemeden hemen sonra**, cihaz uygulamaya zıplamadan önce (veya çok hızlı bir şekilde) script’i ikinci kez çalıştırıp hash kontrolü yapmak zorunda kalırsınız – pratik değil.

**Pratik “zaten yüklü” testi (B1 butonu):**

Bu projede **`BOOTLOADER_FORCE_UPDATE_PIN = 1`** (varsayılan) ile açılışta **B1 (mavi kullanıcı butonu)** basılı tutulursa bootloader doğrudan yükleme moduna girer (uygulama olsa bile atlamaz).

1. Bootloader + uygulama yüklü.
2. **B1’e basılı tutun**, cihazı **reset**leyin → bootloader yükleme modunda kalır (UART bekler).
3. Aynı .bin ile: `python upload_firmware.py -p COM3 ...\I2CDENEME3.bin`  
   → Script hash kontrolü yapar; cihaz “eşleşti” döner → **“Firmware zaten yüklü, yükleme atlanıyor.”** yazar.

B1 kullanmak istemezseniz `bootloader.h` içinde **`BOOTLOADER_FORCE_UPDATE_PIN`** değerini **0** yapın.

---

## Kısa özet

| Adım | Ne yapıyorsunuz? |
|------|-------------------|
| 1 | Bootloader’ı 0x08000000’e derleyip yükleyin (ayrı build + main sadece Bootloader_CheckAndRun). |
| 2 | Uygulama .bin’i 0x08008000 için derleyin (linker + VTOR). |
| 3 | Uygulama bölgesi boşken reset → bootloader yükleme modunda bekler. |
| 4 | `python upload_firmware.py -p COMx uygulama.bin` → yükleme yapılır, cihaz uygulamaya gider. |
| 5 | “Zaten yüklü” için: **B1’e basılı tutup** reset atın → bootloader yükleme modunda kalır. Aynı `upload_firmware.py` komutunu çalıştırın → “Firmware zaten yüklü, yükleme atlanıyor.” |

COM portunu Windows **Cihaz Yöneticisi → Bağlantı noktaları (COM ve LPT)** altından kontrol edin.

---

## Hızlı komut özeti

```bash
# Port ve .bin yolunu kendinize göre değiştirin
cd C:\Users\PC_2255\Desktop\deneme\I2CDENEME3\tools
pip install pyserial

# İlk yükleme (cihaz yükleme modundayken)
python upload_firmware.py -p COM3 ..\Debug\I2CDENEME3.bin

# "Zaten yüklü" testi: B1 basılı + reset, sonra aynı komut
python upload_firmware.py -p COM3 ..\Debug\I2CDENEME3.bin
```
