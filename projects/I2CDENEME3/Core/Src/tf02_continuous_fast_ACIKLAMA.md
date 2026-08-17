# tf02_continuous_fast.c — Kod Açıklaması (Tek Tek)

Bu dosya, `tf02_continuous_fast.c` içindeki her bölümün **ne yaptığını** ve **neden yazıldığını** açıklar.

---

## 1. Include’lar (satır 3–10)

```c
#include "main.h"
#include "tf02_continuous_fast.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
```

- **Ne:** Gerekli başlık dosyalarını ekler.
- **Neden:** `main.h` → HAL/GPIO tanımları; `i2c.h` / `usart.h` / `gpio.h` → I2C, UART, GPIO fonksiyonları; `stdarg` / `stdio` → `LOG()` için `vsnprintf`; `stdint` → `uint8_t`, `uint16_t` gibi sabit genişlikte tamsayılar.

---

## 2. Define’lar (satır 13–22)

```c
#define TFMINI_ADDR_7BIT   0x10u
#define TFMINI_ADDR        (TFMINI_ADDR_7BIT << 1)
```
- **Ne:** Sensörün I2C adresi. 7 bit adres 0x10; HAL 8 bit (7 bit + R/W) istediği için sola 1 kaydırılıyor.
- **Neden:** TF02-Pro / TFmini varsayılan adresi 0x10; tüm I2C çağrılarında bu adres kullanılıyor.

```c
#define TF_WAIT_CFG_MS     100u
#define TF_GETCM_WAIT_MS   0u
```
- **Ne:** `TF_WAIT_CFG_MS` = konfigürasyon komutundan sonra 100 ms bekleme. `TF_GETCM_WAIT_MS` = “mesafe frame’i al” komutundan sonra 0 ms bekleme (test için).
- **Neden:** Konfigürasyon sonrası sensör cevabı için süre vermek; GET_CM sonrası bekleme ise frekansı artırmak için kısaltılabiliyor (0 / 1 / 2 ms deneyebilirsin).

```c
#define PRINT_DELTA_CM     2u
#define MED_WIN            5u
```
- **Ne:** Medyan filtreden gelen mesafe, bir önceki yazdırılan değerden en az **2 cm** farklıysa tekrar yazdırılıyor. Medyan penceresi **5** ölçüm.
- **Neden:** Her frame’i yazdırmak yerine sadece “anlamlı değişim” olduğunda yazdırmak; 5’li medyan ise tek tük hatalı ölçümleri yumuşatıyor.

```c
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;
```
- **Ne:** I2C1 ve USART2’nin HAL tanımları; başka dosyada (i2c.c, usart.c) oluşturuluyor.
- **Neden:** Bu .c dosyası sadece kullanıyor; tanımı burada tekrarlamamak için `extern` ile bildiriyoruz.

---

## 3. LOG() (satır 25–35)

```c
static void LOG(const char *fmt, ...)
{
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n <= 0) return;
  if (n > (int)sizeof(buf)) n = (int)sizeof(buf);
  HAL_UART_Transmit(&huart2, (uint8_t*)buf, (uint16_t)n, HAL_MAX_DELAY);
}
```

- **Ne:** `printf` benzeri; format string ve değişken sayıda argüman alıp metni USART2 üzerinden gönderir.
- **Neden:** Seri monitörde mesafe, hata, istatistik gibi çıktıları tek bir fonksiyonla yazmak; `HAL_UART_Transmit` doğrudan string istediği için önce `vsnprintf` ile `buf`’a yazıyoruz.

---

## 4. TF02_I2C1_BusRecover() (satır 37–65)

- **Ne:** I2C kullanılmadan önce SCL (PB8) ve SDA (PB9) pinlerini GPIO açık-drenaj çıkış yapıp SCL’de 9 clock atıyor, sonra STOP (SDA yükselir → SCL yükselir) üretiyor.
- **Neden:** I2C bazen “takılı” kalır (slave SDA’yı düşük tutar); bu durumda normal I2C donanımı çalışmaz. Manuel clock + STOP ile hattı serbest bırakıyoruz. **I2C init’ten önce** çağrılmalı (main.c’de öyle yapıldı).

---

## 5. TFmini_WaitFound() (satır 67–76)

- **Ne:** Verilen süre (ms) boyunca I2C adres 0x10’da cihaz var mı diye `HAL_I2C_IsDeviceReady` ile bakıyor; bulursa 1, süre biterse 0 döner.
- **Neden:** Sensör henüz hazır değilse veya kablo takılı değilse hemen komut göndermemek; kullanıcıya “cihaz bulunamadı” diyebilmek için önce “var mı?” kontrolü.

---

## 6. tf_checksum() (satır 78–84)

- **Ne:** Verilen byte dizisinin ilk `n_without_checksum` byte’ının toplamının düşük 8 bitini (0xFF ile maskeler) döndürür.
- **Neden:** TF02-Pro komut ve frame’lerinde checksum bu şekilde hesaplanıyor; hem komut gönderirken hem gelen 9 byte frame’i doğrularken kullanılıyor.

---

## 7. tf_send_cmd_get_resp5() (satır 86–104)

- **Ne:** 5 byte’lık komutu checksum’ı hesaplayıp son byte’a yazar, I2C ile gönderir; 100 ms bekler; ardından 5 byte cevap okumaya çalışır ve sonucu LOG’lar.
- **Neden:** OUTPUT ENABLE gibi konfigürasyon komutları bu formatta; sensör bazen cevap vermeyebilir ama komut gitsin diye “gönder + bekle + varsa oku” akışı kullanılıyor.

---

## 8. TFmini_OutputEnable_I2C() (satır 106–112)

- **Ne:** `5A 05 07 01 xx` (checksum’lı) komutunu gönderir; bu “I2C çıkışını aç” komutu.
- **Neden:** Sensörün I2C üzerinden veri üretmesi için bu komutun bir kez gönderilmesi gerekiyor (kılavuza göre).

---

## 9. TFmini_ChecksumOK9() (satır 114–119)

- **Ne:** 9 byte’lık frame’te ilk 8 byte’ın toplamının düşük 8 biti, 9. byte (checksum) ile aynı mı kontrol eder.
- **Neden:** Bozuk veya yanlış frame’leri atmak için; checksum tutmuyorsa bu frame işlenmez (okuma hatası sayılır).

---

## 10. TFmini_ReadFrame_CM() (satır 121–139)

- **Ne:** “9 byte mesafe (cm) frame’i ver” komutunu (5A 05 00 01 60) gönderir; `TF_GETCM_WAIT_MS` kadar bekler; 9 byte okur; frame header 0x59 0x59 ve checksum doğruysa 1, değilse 0 döner.
- **Neden:** Her döngüde tek bir mesafe frame’i almanın standart yolu; bekleme süresi ne kadar kısa olursa o kadar yüksek Hz (0/1/2 ms ile deneyebilirsin).

---

## 11. sort2_u16() ve median5_u16() (satır 141–159)

- **Ne:** `sort2_u16` iki sayıyı küçük–büyük sıralar. `median5_u16` beş sayıyı sıralayıp ortadaki (3.) değeri döndürür (sorting network tarzı karşılaştırmalarla).
- **Neden:** Son 5 mesafe ölçümünün medyanını alıp tek tük aykırı değerleri (sıçramalar) azaltmak; böylece yazdırılan “MED” daha kararlı olur.

---

## 12. TF02_Run_Continuous_Fast() (satır 162–255)

### Başlangıç (satır 164–177)

- **Ne:** “TF02 start”, mod bilgisi ve GET_CM bekleme süresini yazar; 400 ms bekler; sensörü bulana kadar dener; bulursa “FOUND” yazar; OUTPUT ENABLE komutunu gönderir; “Streaming...” yazar.
- **Neden:** Seri çıktının geldiğini ve sensörün hazır olduğunu doğrulamak; sensör I2C çıkışını açmak.

### Değişkenler (satır 179–194)

- **Ne:** Sayaçlar (ok_cnt, err_cnt, change_cnt), son ham/medyan değerler, medyan penceresi (win[], win_i, win_fill), 9 byte frame buffer’ı (f[9]). Bazıları `volatile`.
- **Neden:** Her saniye OK/ERR/Hz/Changes yazdırmak; sadece medyan belirli bir miktar değişince (PRINT_DELTA_CM) satır yazdırmak; debugger’da anlık değer görmek için volatile.

### Ana döngü (satır 195–253)

1. **Frame oku (TFmini_ReadFrame_CM):**
   - Başarılıysa: dist, str, temp çıkar; ok_cnt artır; last_* güncelle.
   - **str > 0:** Pencereye dist ekle; pencere dolduysa medyan hesapla; medyan önceki yazdırılan değerden en az PRINT_DELTA_CM farklıysa (veya ilk yazdırma ise) bir satır LOG’la (RAW D, MED, S, T).
   - **str == 0:** Geçersiz kabul et; “WARN: S=0 (invalid)” yaz.
   - Okuma/checksum/header hatası: err_cnt artır; “ERR: frame #…” yaz.

2. **Her 1 saniyede bir:** Geçen süre ve ok_cnt ile Hz hesapla; “[1s] OK=… ERR=… Rate=… Hz Changes=… LastRAW=… LastMED=… S=… T=…” satırını yaz; sayaçları sıfırla; t_stat0’ı güncelle.

- **Neden:** Sürekli mesafe alıp hem anlık (değişince) hem özet (saniyelik) bilgi vermek; hatalı frame’leri ve S=0 durumunu ayrıca göstermek.

---

## Kısa özet

| Bölüm | Ne yapıyor | Neden |
|--------|------------|--------|
| Define’lar | Adres, bekleme süreleri, medyan/print eşikleri | Tek yerde ayar; hız/duyarlılık kolay değişir |
| LOG | printf gibi UART’a yazma | Tüm seri çıktı tek fonksiyondan |
| BusRecover | SCL’de 9 clock + STOP | I2C takılmasını çözmek |
| WaitFound | Cihaz 0x10’da mı bekle | Kablo/cihaz kontrolü |
| Checksum / SendCmd | Komut checksum + gönder + cevap oku | Protokol gereği |
| OutputEnable | I2C çıkışını aç | Sensörün veri üretmesi için |
| ReadFrame_CM | GET_CM + bekle + 9 byte oku + doğrula | Her döngüde tek mesafe frame’i |
| median5 | 5 değerin ortancası | Aykırı ölçümleri azaltmak |
| Ana döngü | Sürekli oku → medyan → değişince yaz; 1 sn’de özet | Yüksek Hz + anlamlı çıktı + hata takibi |

Bu açıklama dosyası, kodu satır satır veya blok blok takip ederken “ne” ve “neden” sorularını yanıtlamak için kullanılabilir.
