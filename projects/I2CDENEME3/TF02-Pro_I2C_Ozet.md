# TF02-Pro: I2C ile Kullanım ve Saniyedeki Ölçüm Sayısı

## 1. Saniyede ne kadar mesafe verisi? (Frame rate)

**Kılavuz (Technical Specification):**
- **Frame rate: 1 Hz ~ 1000 Hz** (ayarlanabilir, varsayılan **100 Hz**)
- Formül: `2000 / n` (n ≥ 2 tamsayı) → En fazla **1000 ölçüm/saniye**
- Yani I2C üzerinden de teorik olarak saniyede **en fazla 1000** mesafe verisi alınabilir (uygulamada I2C okuma süresi ve 100 ms bekleme bu sayıyı sınırlayabilir)

---

## 2. I2C için yapman gerekenler (User Manual’dan)

### 2.1 Kablo (Table 2-1)
| Renk   | UART      | I2C  |
|--------|-----------|------|
| Kırmızı | VCC       | VCC  |
| Beyaz  | RXD       | **SDA** |
| Yeşil  | TXD       | **SCL** |
| Siyah  | GND       | GND  |

Nucleo’da: **Beyaz → SDA**, **Yeşil → SCL**, 3.3V ve GND.

### 2.2 I2C parametreleri (Table 3-3)
- **Arayüz:** I²C, Slave
- **Max hız:** 400 kbps (Fast mode)
- **Varsayılan adres:** **0x10** (0x01–0x7F aralığında değiştirilebilir)

### 2.3 I2C’ye geçiş (ilk kez)
Sensör fabrikada **UART** modunda. I2C kullanmak için **bir kez** UART ile şu komutları göndermen gerekiyor:

1. **I2C moduna al:** `5A 05 0A 01 6A` (Communication interface setup → 01 = I²C)
2. **Kaydet:** `5A 04 11 6F` (Save settings)
3. **Reset (veya güç kes-aç):** `5A 04 02 60`  
   Sonrasında sensör artık **sadece I2C** ile konuşur (UART’tan cevap gelmez).

Bunu ya **test yazılımındaki CUSTOM COMMAND** ile ya da elindeki Python (UART) scripti ile yapabilirsin.

### 2.4 I2C’de mesafe verisi alma (Table 4-1 “Obtain data frame”)
- **Sadece I2C modunda** geçerli.
- **Komut (master yazar):**
  - 9 byte, cm: `5A 05 00 01 60`
  - 9 byte, mm: `5A 05 00 06 65`
- **Zamanlama (Table 3-4):** Komutu yaz → **100 ms bekle** → Sonra **9 byte oku**.

### 2.5 9 byte veri formatı (Table 3-2)
| Byte   | İçerik     | Açıklama        |
|--------|------------|-----------------|
| 0–1    | 0x59 0x59  | Frame header    |
| 2–3    | Dist_L, Dist_H | Mesafe (cm), little-endian |
| 4–5    | Strength_L/H   | Sinyal gücü 0–65535 |
| 6–7    | Temp_L, Temp_H | Sıcaklık: Temp/8 - 256 (°C) |
| 8      | Checksum   | İlk 8 byte’ın toplamının düşük 8 biti |

---

## 3. Test yazılımı (BENEWAKE LIDAR GUI / BW_TFDS) ne işe yarar?

- Yazılım **sadece UART** (COM port, 115200 baud) kullanıyor; **I2C üzerinden veri almıyor**.
- **Ne yapabilirsin:**
  1. **Frame rate ayarı:** “Frame rate” kutusundan 1–1000 Hz seçebilirsin; “SAVE SETTING” ile sensöre kaydedersin. Böylece I2C’ye geçmeden önce örneğin **1000 Hz** yapıp kaydedebilirsin.
  2. **Effective Points (per sec):** Ekranda saniyedeki nokta sayısını gösterir; frame rate ile aynı (veri geliyorsa).
  3. **I2C moduna geçiş:** “CUSTOM COMMAND” alanına `5A 05 0A 01 6A` yazıp SEND, sonra `5A 04 11 6F` (Save), ardından güç kes-aç veya reset. Bundan sonra sensör I2C modunda olur; GUI’den veri alamazsın, Nucleo I2C ile okuyacak.

**Özet:** Test yazılımı UART ile frame rate ve I2C moduna geçiş ayarlarını yapmak için çok işe yarar; I2C ile asıl veri okuma Nucleo’da yapılacak.

---

## 4. Nucleo ile I2C akışı (özet)

1. **İlk kurulum (bir kez):**  
   USB-TTL ile UART’tan frame rate ayarla (isteğe göre 1000 Hz), I2C moduna geçir (`5A 05 0A 01 6A`), Save, Reset/power cycle.
2. **Nucleo bağlantısı:**  
   SDA → Beyaz, SCL → Yeşil, 3.3V, GND. I2C 400 kHz (Fast mode), pull-up uygun olsun.
3. **Her ölçüm:**  
   - Adres **0x10**’a “Obtain data frame” komutunu yaz: `5A 05 00 01 60`  
   - **100 ms** bekle  
   - Aynı adresten **9 byte** oku ve mesafeyi `Dist = Dist_L + (Dist_H << 8)` ile hesapla.

**I2C hız notu:** Saniyede çok yüksek örnekleme istiyorsan UART daha uygun; I2C ile pratik üst sınır ~10 okuma/saniye (100 ms bekleme yüzünden).

Bu adımları uyguladığında sensörün saniyede verebileceği mesafe verisi sayısı kılavuza göre **en fazla 1000 Hz**; pratikte I2C’de her okuma için 100 ms bekleme kullanıldığı için okuma başına ~100 ms sürecek ve saniyede **yaklaşık 10 okuma** ile sınırlı kalırsın. Daha yüksek hız için kılavuzda I2C için özel “trigger” veya sürekli yayın modu var mı diye tekrar bakmak gerekir; şu anki tabloda “Obtain data frame” tek komut + 100 ms bekleme olarak geçiyor.
