# TF02-Pro I2C Frekans Artırma Önerileri

## 1. GET_CM sonrası delay'i kısalt (en etkili)

Kodunda:
```c
#define TF_GETCM_WAIT_MS   5u     // GET_CM -> frame bekleme
```

**Yapman gereken:** Bu değeri **2** veya **1** yap.

- **2 ms:** Güvenli deneme. 140 Hz → yaklaşık 250–350 Hz beklenir.
- **1 ms:** Sensör 1000 Hz modundaysa frame 1 ms’de hazır; dene. Hata sayısı (ERR) artarsa 2 ms’e çık.

Kılavuzda “100 ms bekle” yazıyor; bu muhafazakar değer. Sen 5 ms ile 140 Hz alıyorsan sensör çok daha erken hazır. 2 ms ile hata artmıyorsa kullanmaya devam et.

---

## 2. Sensör frame rate’ini 1000 Hz yap (bir kez)

TF02-Pro varsayılan 100 Hz. Saniyede daha çok veri için:

- **BENEWAKE LIDAR GUI** (UART, 115200) ile sensörü bağla.
- **Frame rate** kutusundan **1000 Hz** seç, **SAVE SETTING** ile kaydet.
- Sonra I2C’ye geçip Nucleo ile oku.

Bunu yapmazsan, delay’i çok kısaltınca “hazır değil” cevabı gelir, hata artar. 1000 Hz’e ayarlayınca 1–2 ms bekleme yeterli olur.

---

## 3. Kodda yapılacak tek satırlık değişiklik

`main.c` (veya TFmini continuous kullandığın dosya) içinde:

**Eski:**
```c
#define TF_GETCM_WAIT_MS   5u     // GET_CM -> frame bekleme (2..10ms deneyebilirsin)
```

**Yeni (önerilen):**
```c
#define TF_GETCM_WAIT_MS   2u     // GET_CM -> frame bekleme (1..2ms yeterli, 1000Hz modunda)
```

Derle, yükle, 1 saniyelik özette **Rate=... Hz** değerine bak. ~250–350 Hz civarı görebilirsin. ERR artıyorsa 3 ms dene.

---

## 4. I2C hızı

TF02-Pro kılavuzu: **maksimum 400 kbps (Fast mode)**. I2C hızını artırma; donanım sınırı.

---

## 5. Özet tablo

| Yapılan işlem              | Beklenen etki        |
|---------------------------|----------------------|
| `TF_GETCM_WAIT_MS = 2`    | ~250–350 Hz          |
| `TF_GETCM_WAIT_MS = 1`    | Daha yüksek, hata riski |
| Sensör 1000 Hz’e ayarlı   | Delay kısa olsa da daha az hata |
| I2C 400 kHz’te bırakmak   | Doğru, artırma       |

En pratik adım: **TF_GETCM_WAIT_MS’i 2 yap**, sensörü **1000 Hz**’e ayırıp kaydet, sonra ölç. Gerekirse 1 ms’e inip ERR’e bak.
