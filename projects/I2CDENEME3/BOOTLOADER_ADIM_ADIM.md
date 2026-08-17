# STM32F446RE Bootloader – Adım Adım Algoritma

## “Dosya” Ne Anlama Geliyor?

Mikroişlemcide **dosya sistemi yoktur**. “Yüklenecek dosya (.hex/.bin) **mevcut mu?**” sorusu şu anlama gelir:

- **Hedef:** Uygulama bölgesinde (0x08008000) **geçerli bir firmware imajı** var mı?
- **Var** → Bu imaj “dosya mevcut” sayılır → Yükleme **atlanır**, doğrudan uygulamaya geçilir.
- **Yok** → “Dosya yok” → Yükleme süreci **başlatılır** (UART ile veri alınıp flash’a yazılır).

Yani “dosya” = flash’taki uygulama firmware’i; kontrol, `Bootloader_IsFirmwarePresent()` ile yapılır.

---

## Adım 1: Algoritma Özeti

```
[ Açılış ]
     |
     v
Minimal donanım init (RCC, GPIO, UART)
     |
     v
┌─────────────────────────────────────────┐
│  Uygulama bölgesinde firmware var mı?    │
│  (0x08008000: SP + Reset geçerli mi?)    │
└─────────────────────────────────────────┘
     |
     ├── EVET (dosya/firmware VAR)
     |        → Yükleme işlemini ATLA
     |        → Doğrudan uygulamaya zıpla (Jump)
     |        → BİTİŞ
     |
     └── HAYIR (dosya/firmware YOK)
              → Yükleme sürecini BAŞLAT
              → UART üzerinden komut bekle
              → Gelen .bin verisini flash’a yaz
              → Yazma bitince uygulamaya zıpla veya reset
```

---

## Adım 2: “Dosya Var mı?” Kontrolü – Hafıza Taraması ve CRC Eşleşmesi

“Var” kararı **iki değerin eşleşmesine** dayanır:

1. **Hafızayı tara:** Uygulama bölgesinde (0x08008000’den itibaren, imza 8 byte hariç) **CRC32** hesaplanır.
2. **Saklı değer:** Yükleme sırasında aynı bölgenin CRC32’si hesaplanıp **0x08008204** adresine yazılır (hex/bin imajı ile aynı algoritma).
3. **Eşleşme:** Açılışta hesaplanan CRC, saklı CRC ile **aynıysa** → firmware **var** ve doğru kabul edilir; **farklıysa** veya saklı değer yoksa → **yok** → yükleme moduna girilir.

Ek kontroller (güvenlik için):

| Kontrol | Açıklama |
|--------|----------|
| İlk kelime (SP) | 0x08008000 **0xFFFFFFFF** olmamalı (boş flash). |
| İkinci kelime (Reset) | 0x08008004 **0xFFFFFFFF** olmamalı. |
| SP geçerliliği | İlk kelime **0x2000xxxx** olmalı (RAM adresi). |
| İsteğe bağlı Magic | `BOOTLOADER_USE_MAGIC=1` ise 0x08008200’de sabit imza aranır. |
| **Hash (MD5)** (`BOOTLOADER_USE_HASH=1`, varsayılan) | Hafıza taraması MD5’i (ilk 512 + 20 byte atla + 512 byte) == 0x08008204’teki saklı 16 byte MD5. “Var mı yok mu” hash eşleşmesine göre. |
| **CRC** (`BOOTLOADER_USE_CRC=1`, eski) | İsteğe bağlı CRC modu; `BOOTLOADER_USE_HASH=0` yapıp CRC kullanılabilir. |

Sonuç:

- **Tüm kontroller geçer** → Firmware var → yükleme atlanır.
- **Bir tanesi bile geçmez** (SP/Reset boş, CRC eşleşmez vb.) → Firmware yok/geçersiz → yükleme moduna girilir.

---

## Adım 3: Ana Akış – CheckAndRun

`Bootloader_CheckAndRun()` akışı:

1. `Bootloader_IsFirmwarePresent()` çağrılır.
2. **Firmware varsa:** `Bootloader_JumpToApplication()` ile uygulamaya zıplanır (yükleme yapılmaz).
3. **Firmware yoksa:** `Bootloader_EnterUpdateMode()` çağrılır; UART’tan veri alınıp flash’a yazılır, gerekirse sonra uygulamaya zıplanır.

Böylece “dosya varsa atla, yoksa yükle” davranışı tek noktadan yönetilir.

---

## Adım 4: Yükleme Modu – UART Protokolü (Özet)

Firmware yokken girilen `Bootloader_EnterUpdateMode()` içinde:

1. **Komut bekle:** Host tek byte gönderir (örn. `0x01` = “yükleme başlat”).
2. **Boyut al:** Host 4 byte (little-endian) gönderir = yüklenecek .bin boyutu.
3. **Veri al:** Host, boyut kadar ham .bin verisi gönderir.
4. **Flash işlemleri:**
   - Uygulama bölgesi (0x08008000’den itibaren ilgili sektörler) silinir.
   - Gelen veri bu adresten itibaren flash’a yazılır.
5. **Bitiş:** Hata yoksa uygulamaya zıplanır veya NVIC_SystemReset() ile yeniden başlatılır.

PC tarafında .hex kullanıyorsanız, önce .hex’i .bin’e çevirip (ve 0x08008000’e göre konumlandırıp) bu protokolle göndermeniz gerekir.

**UART bağımlılığı:** `Bootloader_EnterUpdateMode()` `huart2` (USART2) kullanır. Bootloader’ı ayrı bir proje olarak derliyorsanız, UART init’i o projede yapılmalı ve `usart.c`/`usart.h` (veya kendi UART modülünüz) eklenmelidir.

**PC yazılımı (ArduPilot tarzı):** `tools/upload_firmware.py` hex/bin yüklerken önce **0x02 (CRC kontrol)** gönderir; cihaz "zaten aynı" derse **"Firmware zaten yüklü, yükleme atlanıyor."** yazar ve yükleme yapmaz. Ayrıntı: `tools/README_YUKLEYICI.md`.

---

## Adım 5: Bellek Haritası (STM32F446RET6)

| Bölge       | Adres        | Boyut  | Açıklama                    |
|------------|--------------|--------|-----------------------------|
| Bootloader | 0x0800 0000  | 32 KB  | Bootloader kodu (sektör 0–1) |
| Uygulama   | 0x0800 8000  | 480 KB | Ana uygulama (sektör 2–7)   |

STM32F446’da sektör 2 = 0x08008000 (uygulama başlangıcı). Yükleme modunda sadece uygulama sektörleri (2–7) silinir; bootloader (0–1) dokunulmaz.

---

## Adım 6: Özet Tablo

| “Dosya” (firmware) | Bootloader davranışı |
|--------------------|----------------------|
| **Var** (geçerli SP + Reset) | Yükleme **atlanır** → doğrudan uygulamaya zıplanır. |
| **Yok** (boş/geçersiz)       | Yükleme **başlatılır** → UART’tan .bin alınır, flash’a yazılır, sonra uygulamaya geçilir. |

Bu dokümandaki adımlar, mevcut `bootloader.h` / `bootloader.c` ve `BOOTLOADER_TASARIM.md` ile uyumludur; tek eklenen kısım, “dosya yok” durumunda çalışan **yükleme modu**nun (UART + flash yazma) tam implementasyonudur.
