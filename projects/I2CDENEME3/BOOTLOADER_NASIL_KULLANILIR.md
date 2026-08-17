# Bootloader – Her Şey Bir Arada Rehber

Bu projede **iki ayrı firmware** var. Hangisi nerede, nasıl derlenir, nasıl yüklenir – hepsi burada.

---

## 1. İki firmware ne?

| Firmware      | Nerede çalışır? | Ne yapar? |
|---------------|------------------|-----------|
| **Bootloader** | 0x08000000 (ilk 32 KB) | Açılışta “uygulama var mı?” bakar (MD5). Varsa uygulamaya atlar; yoksa UART’ta bekler. PC’den gelen .bin’i flash’a yazar. |
| **Uygulama**   | 0x08008000 (480 KB)   | Asıl proje (TF-Mini, I2C vb.). Normal `main.c` ile derlenen program. |

İkisi **ayrı ayrı derlenir**, **ayrı ayrı yüklenir**. Önce bootloader, sonra (isterseniz) uygulama.

---

## 2. Hangi dosya neye ait?

### Hep birlikte kullanılan (ortak)

- `Core/Inc/bootloader.h` – Bootloader sabitleri, adresler.
- `Core/Src/bootloader.c` – Bootloader mantığı (MD5, flash, UART). **Sadece bootloader build’inde** kullanılır; uygulama build’inde bu dosya **derlenmez** (exclude edin).
- `Core/Src/usart.c`, `Core/Inc/usart.h` – UART. Bootloader bunu kullanır; uygulama da kullanıyorsa her iki build’de de olur.
- `Core/Src/gpio.c`, `Core/Inc/gpio.h` – GPIO. İkisinde de gerekebilir.

### Sadece bootloader (0x08000000) için

- `Core/Src/main_bootloader.c` – Bootloader’ın **main**’i. Sadece init + `Bootloader_CheckAndRun()`.
- `STM32F446RETX_BOOT.ld` – Bootloader **linker script**: FLASH = 0x08000000, 32K.

**Bootloader build’inde:** `main.c` **olmasın** (exclude), `main_bootloader.c` **olsun**. Linker = `STM32F446RETX_BOOT.ld`.

### Sadece uygulama (0x08008000) için

- `Core/Src/main.c` – Uygulamanın **main**’i (TF-Mini, I2C vb.).
- `STM32F446RETX_FLASH.ld` – Uygulama **linker script**: FLASH = **0x08008000**, 480K (bootloader ile kullanacaksanız UYGULAMA_BOOTLOADER_AYARLARI.md’deki gibi değiştirin).

**Uygulama build’inde:** `main.c` **olsun**, `main_bootloader.c` **olmasın** (exclude). Linker = `STM32F446RETX_FLASH.ld` (ORIGIN = 0x08008000).

### PC tarafı (cihaza yükleme)

- `tools/upload_firmware.py` – Hex/bin’i UART ile atar; “zaten yüklü” kontrolü yapar.
- `tools/README_YUKLEYICI.md`, `tools/HEX_BIN_NEREDEN.md` – Kullanım ve hex/bin nereden.

---

## 3. İki build nasıl ayarlanır? (STM32CubeIDE)

### A) Uygulama build’i (zaten var gibi kullan)

- **Build Configuration:** Debug veya Release.
- **Linker script:** `STM32F446RETX_FLASH.ld`.
- **FLASH:** ORIGIN = **0x08008000**, LENGTH = 480K (bootloader ile kullanacaksanız).
- **Kaynak:** `main.c` derlensin, `main_bootloader.c` **exclude**.
- İsterseniz `bootloader.c` bu konfig’de **exclude** (uygulama bootloader kodunu taşımaz).

### B) Bootloader build’i (yeni konfig)

1. **Project → Build Configurations → Manage… → New** → İsim: **Bootloader**.
2. **C/C++ Build → Settings:**
   - **Linker → General → Linker script:** `STM32F446RETX_BOOT.ld` (path: `../STM32F446RETX_BOOT.ld` veya proje kökü).
3. **Kaynakları ayır:**
   - **Sources** içinde `main.c` → sağ tık → **Resource Configurations → Exclude from Build** → **Bootloader** işaretli olsun.
   - `main_bootloader.c` → **Sadece Bootloader** konfig’de derlensin (Debug/Release’ten exclude, Bootloader’da include).
   - `bootloader.c` → **Sadece Bootloader** konfig’de derlensin (Uygulama build’inde exclude).
4. Build → **Active configuration: Bootloader** seçip derleyin. Çıktı: `Debug/I2CDENEME3.elf` (ve .bin) → bunu **0x08000000**’e yükleyin.

---

## 4. Yükleme sırası (ilk kurulum)

1. **Bootloader’ı yükle:** Bootloader build’ini derleyin → `I2CDENEME3.bin` (veya .hex) → ST-Link / STM32CubeProgrammer ile **0x08000000**’e yazın.
2. **Uygulama bölgesi boş kalsın** (0x08008000’e henüz bir şey yazmayın; veya sektör 2–7’yi silin).
3. **Reset** atın → Bootloader çalışır, uygulama olmadığı için UART’ta bekler.
4. **Uygulamayı UART ile atın:**  
   `python tools/upload_firmware.py -p COMx Debug/I2CDENEME3.bin`  
   (Uygulama build’inden aldığınız .bin; COM port kendi cihazınız.)
5. Yükleme bitince cihaz uygulamaya geçer.

Sonraki sefer: Cihaz açılınca bootloader “uygulama var” deyip doğrudan uygulamaya atlar. Tekrar firmware atmak isterseniz Python script’i kullanırsınız; “zaten yüklü” derse atlamaz.

---

## 5. Kısa özet tablo

| Ne?              | Nerede? / Hangi build? |
|------------------|------------------------|
| Bootloader kodu  | `bootloader.c` + `main_bootloader.c` → **Bootloader** build, linker **BOOT.ld**, çıktı **0x08000000**. |
| Uygulama kodu    | `main.c` + diğer uygulama dosyaları → **Debug/Release** build, linker **FLASH.ld** (0x08008000), çıktı **0x08008000**. |
| Bootloader yükleme | ST-Link ile boot .bin’i **0x08000000**’e yaz. |
| Uygulama yükleme   | Python `upload_firmware.py` ile uygulama .bin’i UART’tan at (bootloader alır, 0x08008000’e yazar). |

---

## 6. “Birbirine girmemesi” için kontrol listesi

- [ ] Bootloader build’inde **sadece** `main_bootloader.c` + `bootloader.c` + HAL, gpio, usart, startup.
- [ ] Uygulama build’inde **sadece** `main.c` (main_bootloader ve bootloader **exclude**).
- [ ] Boot linker = **STM32F446RETX_BOOT.ld** (0x08000000, 32K).
- [ ] Uygulama linker = **STM32F446RETX_FLASH.ld** (0x08008000, 480K).
- [ ] Önce 0x08000000’e bootloader, sonra UART ile 0x08008000’e uygulama.

Bu rehbere göre ilerlerseniz iki firmware birbirine karışmaz; biri “giriş” (bootloader), biri “ana program” (uygulama) olur.
