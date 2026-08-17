# STM32F446 Bootloader – Algoritma (UART, Hash/MD5, SD yok)

SD kart kullanılmıyor. Yükleme **UART** (USART2) ile yapılır; “var mı yok mu” kararı **flash’taki MD5** ile verilir.

---

## Akış şeması

```
                    [RESET / GÜÇ VER]
                               │
                               ▼
              [Sistem başlatma: HAL_Init, saat, GPIO, UART]
                               │
                               ▼
                  ┌────────────────────────────┐
                  │  B1 basılı mı? (test modu)  │
                  │  (BOOTLOADER_FORCE_UPDATE)  │
                  └────────────────────────────┘
                        │            │
                       HAYIR        EVET
                        │            │
                        │            ▼
                        │     [Yükleme moduna gir]
                        │     (UART’tan komut bekle)
                        │            │
                        ▼            │
          ┌──────────────────────────────┐
          │  Flash’ta uygulama var mı?    │
          │  (SP/Reset geçerli + MD5)     │
          └──────────────────────────────┘
                │                    │
               HAYIR                EVET
                │                    │
                ▼                    ▼
     [Yükleme moduna gir]    [VTOR = 0x08008000]
     (UART’tan komut bekle)           │
                │                    ▼
                │           [Uygulamaya zıpla]
                │
                ▼
     ┌─────────────────────────────────────────┐
     │  YÜKLEME MODU (döngü)                    │
     │  • 0x02 + 16 byte MD5 gelirse:           │
     │    saklı MD5 ile karşılaştır;            │
     │    eşleşirse 0x01 (zaten yüklü),        │
     │    değilse 0x00 dön.                     │
     │  • 0x01 + 4 byte boyut + veri gelirse:  │
     │    Sektör 2–7’yi sil,                   │
     │    gelen .bin’i 0x08008000’e yaz,        │
     │    imajın MD5’ini hesapla,               │
     │    0x08008200’e magic+MD5 yaz,           │
     │    uygulamaya zıpla.                     │
     └─────────────────────────────────────────┘
```

---

## “Var mı yok mu” kriteri (SD yok, sadece flash)

- **Vektör tablosu:** 0x08008000’deki ilk iki kelime (SP, Reset) 0xFFFFFFFF olmamalı; SP 0x2000xxxx olmalı.
- **MD5:** 0x08008204’te saklı 16 byte MD5, flash’taki imaj bölgesinin (512 + 20 atla + 512 byte) MD5’i ile **eşleşmeli**.
- Hepsi sağlanıyorsa → **var** → yükleme atlanır, uygulamaya zıplanır. Biri bile sağlanmıyorsa → **yok** → yükleme modu.

---

## Bellek kullanımı

| Bölge        | Adres        | İçerik |
|-------------|--------------|--------|
| Bootloader  | 0x08000000   | Bootloader kodu (32 KB) |
| Uygulama    | 0x08008000   | Ana uygulama (.bin buradan başlar) |
| İmza / MD5  | 0x08008200   | 4 byte magic + 16 byte MD5 (hash burada saklanır) |

MD5, **yükleme sırasında** (UART’tan gelen .bin flash’a yazıldıktan sonra) hesaplanıp 0x08008204’e yazılır. Açılışta aynı bölge tekrar hesaplanıp bu değerle karşılaştırılır.

---

## Proje yapısı (SD yok, UART + Flash)

```
I2CDENEME3/                    ← Uygulama projesi (0x08008000)
├── Core/
│   ├── Inc/
│   │   ├── bootloader.h       ← Bootloader API + sabitler
│   │   └── ...
│   └── Src/
│       ├── main.c             ← Uygulama (TF-Mini vb.)
│       ├── bootloader.c       ← Bootloader: MD5, flash okuma/yazma, UART protokolü
│       ├── usart.c            ← UART (bootloader bunu kullanır)
│       └── ...
├── tools/
│   ├── upload_firmware.py     ← PC: hex/bin’i UART ile atar, “zaten yüklü” kontrolü
│   ├── README_YUKLEYICI.md
│   └── HEX_BIN_NEREDEN.md
├── STM32F446RETX_FLASH.ld    ← Uygulama: ORIGIN = 0x08008000 (uygulama build’inde)
└── BOOTLOADER_*.md            ← Tasarım, test, algoritma dokümanları
```

**Bootloader** ayrı bir build olarak **0x08000000**’e derlenir; kendi `main.c`’i sadece `HAL_Init`, saat, GPIO, UART ve **`Bootloader_CheckAndRun()`** çağırır. SD kart, FatFS veya `flash_if` modülü **yok**; tüm flash erişimi ve MD5 `bootloader.c` içinde (HAL_FLASH + kendi MD5 kodu).

---

## Özet

- **Giriş:** Reset → init → (isteğe bağlı B1) → “Flash’ta uygulama var mı?” (SP/Reset + MD5).
- **Var:** Uygulamaya zıpla (0x08008000).
- **Yok:** UART’ta bekle; 0x02 ile hash kontrolü (PC “zaten yüklü” öğrenir), 0x01 ile .bin al → sektör 2–7 sil, yaz, MD5 yaz, zıpla.
- **SD kart yok;** tüm yükleme UART + `upload_firmware.py` ile.

Bu akış, şu an kodda olan UART + hash tabanlı bootloader ile birebir uyumludur.

---

## Oluşturulan kodlar

| Dosya | Açıklama |
|-------|----------|
| `Core/Src/main_bootloader.c` | Sadece bootloader çalıştıran main: HAL_Init, saat, GPIO, UART → `Bootloader_CheckAndRun()`. Boot build’de **bunu** kullanın, `main.c` değil. |
| `Core/Src/bootloader.c` | MD5, flash okuma/yazma, “var mı” kontrolü, UART protokolü (0x01 / 0x02). |
| `Core/Inc/bootloader.h` | Adresler, komut sabitleri, `Bootloader_CheckAndRun()` vb. |
| `STM32F446RETX_BOOT.ld` | Bootloader linker: FLASH **0x08000000**, **32K**. Boot build’de bu script kullanılır. |

**Boot build nasıl yapılır (STM32CubeIDE):**

1. Yeni **Build Configuration** ekleyin (örn. “Bootloader”).
2. Bu konfigürasyonda **Linker Script** olarak `STM32F446RETX_BOOT.ld` seçin.
3. **Source** olarak `main.c` yerine sadece `main_bootloader.c` derlensin (main.c’yi Boot configuration’dan çıkarın veya exclude edin, main_bootloader.c’yi ekleyin).
4. Derleyip çıkan `.bin`’i **0x08000000**’e yükleyin (ST-Link).

---

## Algoritma değerlendirmesi

**Güçlü yönler**

- **Basit ve tek kanal:** SD yok; UART tek arayüz. Kablo ve yazılım tarafı sade.
- **Hash ile doğrulama:** Sadece vektör tablosu değil, MD5 ile imaj bütünlüğü kontrol ediliyor; yanlış/bozuk firmware’e atlama riski azalıyor.
- **“Zaten yüklü” davranışı:** PC tarafında aynı imajı tekrar atmak gereksiz yere flash’ı silip yazmıyor; hem süre hem ömür açısından iyi.
- **Test kolaylığı:** B1 ile zorla yükleme modu, “zaten yüklü” ve yükleme testlerini hızlı yapmayı sağlıyor.
- **Bellek ayrımı net:** 32K bootloader, kalanı uygulama; adresler sabit, hatası az.

**İsteğe bağlı iyileştirmeler**

- **Timeout:** Uygulama yokken UART’ta süresiz beklemek yerine, belirli süre (örn. 5–10 sn) komut gelmezse tekrar “var mı?” kontrolü veya kısa LED/indikasyon düşünülebilir. Şu anki “sonsuz bekleme” da geçerli bir tercih.
- **Yazma sonrası doğrulama:** İlk yüklemeden sonra flash’tan bir kez daha okuyup MD5 karşılaştırması yapılabilir; hata durumunda tekrar yazma veya NAK ile PC’ye bildirim verilebilir.

Özet: Algoritma UART tabanlı, hash doğrulamalı ve SD’siz kullanım için tutarlı ve uygulanabilir; mevcut kod bu akışı karşılıyor.
