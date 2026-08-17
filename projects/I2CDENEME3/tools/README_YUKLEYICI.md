# Firmware yükleyici (STM32F446RE UART bootloader)

Hex veya bin dosyasını STM32 bootloader'a **UART (USART2)** üzerinden, **256 byte buffer'lar** halinde yükler. **Yüklemeden önce** cihazdaki **MD5 hash** ile dosyanın MD5'ini karşılaştırır; aynıysa yüklemez.

Tasarım özeti: [BOOTLOADER_YUKLEME_TASARIMI.md](../BOOTLOADER_YUKLEME_TASARIMI.md).

## Davranış

- **Dosya zaten yüklü** (MD5 eşleşiyor) → `Firmware zaten yüklü (MD5 eşleşti), yükleme atlanıyor.` yazar, yükleme yapılmaz.
- **Dosya yok / farklı** → Yükleme başlar; veri buffer'lar halinde STM32'nin uygulama sektörüne (0x08008000) gönderilir, sonunda `Yükleme tamamlandı.` yazar.

## Gereksinim

- Python 3
- `pyserial`: `pip install -r requirements.txt` veya `pip install pyserial`

## Kullanım

```bash
# Varsayılan port COM3, 115200 baud
python upload_firmware.py firmware.bin

# .hex dosyası, port ve baud
python upload_firmware.py -p COM5 -b 115200 firmware.hex

# MD5 kontrolü yapma, her zaman yükle (--force)
python upload_firmware.py --force firmware.bin

# Hesaplanan MD5'i <firmware>.md5 dosyasına kaydet
python upload_firmware.py --save-md5 firmware.bin

# Harici .md5 dosyası ile "zaten var mı" kontrolü (32 karakter hex)
python upload_firmware.py --md5-file firmware.bin.md5 firmware.bin
```

## Parametreler

| Parametre | Açıklama |
|-----------|----------|
| `file` | .hex veya .bin dosyası |
| `-p`, `--port` | Seri port (varsayılan: COM3) |
| `-b`, `--baud` | Baud (varsayılan: 115200) |
| `--force` | MD5 kontrolü yapmadan doğrudan yükle |
| `--md5-file FILE` | "Zaten var" kontrolü için harici .md5 dosyası (32 karakter hex) |
| `--save-md5 [FILE]` | Hesaplanan MD5'i dosyaya yaz (dosya verilmezse `<firmware>.md5`) |

Cihazın **bootloader modunda** (uygulama yoksa veya B1 ile zorla yükleme moduna alındıysa) ve **USART2** portuna bağlı olması gerekir.
