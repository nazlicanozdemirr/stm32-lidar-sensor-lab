# STM32F446RE Bootloader – Yükleme Tasarımı (UART, MD5, Hex/Bin)

Bu doküman, mikroişlemciye yüklenecek dosyanın (.hex / .bin) **mevcut olup olmadığının** kontrolü, **varsa işlemin atlanması**, **yoksa UART üzerinden buffer’lar halinde yükleme** ve **MD5 hash tabanlı doğrulama** akışını açıklar. Hedef: **STM32F446RE**, **STM32CubeIDE**, **USART2**.

---

## 1. Genel Akış

```
[.hex / .bin dosyası]  →  Python script (MD5 hesapla)
                                ↓
                    UART (USART2, 115200)  ←→  STM32 Bootloader
                                ↓
              CMD_CHECK + 16 byte MD5  →  Cihaz saklı MD5 ile karşılaştırır
                                ↓
              ┌─────────────────┴─────────────────┐
              │                                   │
        RSP_ALREADY (0x01)                 RSP_NEED_LOAD (0x00)
        “Dosya zaten yüklü”                 “Yükleme gerekli”
              │                                   │
              ▼                                   ▼
        Yükleme atlanır              CMD_START → boyut → buffer’lar
                                                          ↓
                                              Flash sektör 2–7’ye yaz
                                                          ↓
                                              MD5 hesapla, 0x08008204’e yaz
                                                          ↓
                                              Uygulamaya zıpla
```

- **Dosya var mı?** = Cihazdaki saklı MD5 ile, gönderilecek imajın MD5’i **aynı mı?**
  - Aynıysa → işlem **atlanır** (yükleme yapılmaz).
  - Farklıysa veya cihazda imaj yoksa → **yükleme süreci başlar**.

---

## 2. MD5 Tabanlı “Var / Yok” Kararı

- **Hash bölgesi (bootloader ile aynı):**
  - İmajın **ilk 512 byte**’ı
  - **20 byte atla** (0x08008200: magic + MD5 alanı)
  - Sonraki **512 byte**
  - Toplam **1024 byte** üzerinden MD5 hesaplanır.

- **Saklama:** MD5 (16 byte), flash’ta **0x08008204** adresinde (magic’ten sonra).

- **Python:** Hem `.hex` hem `.bin` için bu 1024 byte’lık bölge aynı şekilde oluşturulup MD5 hesaplanır; bu değer `CMD_CHECK` ile cihaza gönderilir. Cihaz kendi flash’ından hesapladığı MD5 ile karşılaştırır.

- **İsteğe bağlı:** Harici bir **.md5** dosyası (32 karakter hex string) ile de kontrol yapılabilir; script hem imajdan hem de bu dosyadan MD5 kullanacak şekilde genişletilebilir.

---

## 3. Bellek ve Sektörler (STM32F446RE)

| Bölge        | Adres        | Açıklama |
|-------------|--------------|----------|
| Bootloader  | 0x08000000   | 32 KB (linker: `STM32F446RETX_BOOT.ld`) |
| Uygulama    | 0x08008000   | Hex/Bin bu adresten başlar (Sektör 2) |
| İmza / MD5  | 0x08008200   | 4 byte magic + 16 byte MD5 |

- **Silinen sektörler:** 2–7 (uygulama bölgesi).
- **Maksimum imaj boyutu:** 480 KB (`BOOTLOADER_APP_MAX_SIZE`).

---

## 4. UART Protokolü (bootloader.h ile uyumlu)

- **Port:** USART2 (PA2-TX, PA3-RX), **115200 8N1**.

| Komut / Yanıt | Değer | Açıklama |
|---------------|-------|----------|
| CMD_START     | 0x01  | Yükleme başlat; ardından 4 byte boyut (LE) + ham binary (buffer’lar). |
| CMD_CHECK     | 0x02  | Hash kontrolü; ardından 16 byte MD5. Cihaz “zaten yüklü” veya “yükle gerekli” döner. |
| ACK           | 0x06  | İşlem OK. |
| NAK           | 0x15  | Hata. |
| RSP_ALREADY   | 0x01  | Firmware zaten yüklü (yükleme atlansın). |
| RSP_NEED_LOAD | 0x00  | Yükleme gerekli. |

**Yükleme sırası:**

1. **(İsteğe bağlı – “dosya var mı” kontrolü)**  
   `CMD_CHECK` (1 byte) + 16 byte MD5 → Cihaz 1 byte: `RSP_ALREADY` veya `RSP_NEED_LOAD`.

2. **Yükleme:**  
   `CMD_START` (1 byte) → ACK → 4 byte boyut (little-endian) → ACK → veri **256 byte’lık buffer’lar** halinde. Son blok 256’dan kısa olabilir; cihaz 4 byte’a tamamlar (0xFF).

3. **Bitiş:**  
   Tüm veri alındıktan sonra cihaz flash’a MD5 yazar, ACK gönderir ve uygulamaya zıplar.

---

## 5. Python Script’in Rolü

- **Girdi:** `.hex` veya `.bin` dosyası (ve isteğe bağlı `.md5`).
- **İşlemler:**
  1. **.hex** ise: Intel HEX parse edilir, 0x08008000’den itibaren binary çıkarılır.
  2. **.bin** ise: Dosya olduğu gibi 0x08008000’e yazılacak imaj kabul edilir (boyut &lt; 480 KB).
  3. İmajın **MD5’i** hesaplanır (yukarıdaki 1024 byte’lık bölge ile).
  4. Seri port (UART) açılır.
  5. **Dosya var mı?** için `CMD_CHECK` + 16 byte MD5 gönderilir.
  6. Yanıt **RSP_ALREADY** ise: “Firmware zaten yüklü, yükleme atlanıyor.” denir ve çıkılır.
  7. **RSP_NEED_LOAD** (veya timeout) ise: `CMD_START` gönderilir, 4 byte boyut, ardından veri **256 byte’lık buffer’lar** halinde gönderilir.
  8. Son ACK alınırsa yükleme tamamlanır.

- **Bağımlılık:** `pyserial` (örn. `tools/requirements.txt` içinde).

---

## 6. STM32CubeIDE Tarafı

- **Bootloader build:** 0x08000000, 32 KB → `STM32F446RETX_BOOT.ld`, `main_bootloader.c`.
- **Uygulama build:** 0x08008000 → uygulama linker script’i (ORIGIN = 0x08008000).
- **Hex/Bin çıktı:** Uygulama projesinden üretilen `.hex` / `.bin` doğrudan Python script’e verilir; script gerekirse HEX’i binary’e çevirir ve buffer’lar halinde UART’tan gönderir.

Bu tasarım, mevcut `bootloader.c` / `bootloader.h` ve `tools/upload_firmware.py` ile uyumludur; MD5 tabanlı “dosya var mı / yok mu” kararı ve “varsa atla, yoksa yükle” akışı bu dokümandaki gibi çalışır.
