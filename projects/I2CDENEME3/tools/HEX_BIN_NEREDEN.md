# Hex / Bin dosyası nereden gelir, nereye atacaksınız?

## 1. Hex veya bin dosyası nerede oluşur?

Projeyi **STM32CubeIDE** (veya Makefile) ile **derlediğinizde** çıktı şu klasörlerde oluşur:

| Build tipi | Klasör        | Dosyalar |
|------------|---------------|----------|
| Debug      | `I2CDENEME3\Debug\`  | `I2CDENEME3.elf`, `I2CDENEME3.hex`, `I2CDENEME3.bin` |
| Release    | `I2CDENEME3\Release\`| (aynı isimler) |

Yani:
- **Hex:** `C:\Users\PC_2255\Desktop\deneme\I2CDENEME3\Debug\I2CDENEME3.hex`
- **Bin:** `C:\Users\PC_2255\Desktop\deneme\I2CDENEME3\Debug\I2CDENEME3.bin`

Önce projeyi **Build** edin (Ctrl+B veya Project → Build Project). Sonra bu dosyalardan birini kullanacaksınız.

---

## 2. Komutu nereden çalıştıracaksınız?

**Komut satırını** (PowerShell veya CMD) açıp:

- Ya **tools** klasörüne gidip script’i oradan çalıştırırsınız,
- Ya da proje kökünden `tools\upload_firmware.py` yolunu verirsiniz.

Örnek (tools klasöründeyken):

```bash
cd C:\Users\PC_2255\Desktop\deneme\I2CDENEME3\tools
python upload_firmware.py -p COM3 ..\Debug\I2CDENEME3.hex
```

Örnek (proje kökündeyken):

```bash
cd C:\Users\PC_2255\Desktop\deneme\I2CDENEME3
python tools\upload_firmware.py -p COM3 Debug\I2CDENEME3.hex
```

**COM portunu** (COM3, COM4, …) kendi cihazınıza göre değiştirin (Cihaz Yöneticisi → Bağlantı noktaları).

---

## 3. Kısa özet

| Ne?        | Nerede? |
|-----------|---------|
| Hex/bin   | Projeyi build ettikten sonra **Debug\** veya **Release\** içinde (örn. `I2CDENEME3.hex`, `I2CDENEME3.bin`) |
| Komut     | **tools** klasöründe veya proje kökünde: `python upload_firmware.py -p COMx <hex veya bin dosyasının yolu>` |
| Cihaz     | Bootloader yükleme modunda, aynı UART (USART2, 115200) ile PC’ye bağlı olmalı |

Script hem **.hex** hem **.bin** kabul eder; ikisiyle de aynı şekilde atarsınız.
