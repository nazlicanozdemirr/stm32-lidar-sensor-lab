# STM32F446RE Bootloader – Adım Adım Test Rehberi

Bu rehber, bootloader ve UART ile firmware yüklemeyi **sıfırdan** ve **tek tek** nasıl test edeceğinizi anlatır. Her adımda **neyi, neden ve nasıl** yapacağınız yazıyor.

---

## Bölüm 1: Başlamadan Önce

### 1.1 Donanım

- **Kart:** STM32F446RE (Nucleo-F446RE veya kendi devreniz).
- **Bağlantı:** Kart PC’ye bağlı olmalı:
  - **ST-Link USB** (Nucleo’da tek USB) → hem programlama hem (Nucleo’da) sanal COM port için kullanılır.
  - Veya ayrı **USB-UART** modülü: STM32 **PA2 (TX)**, **PA3 (RX)** ve **GND**, modülün **RX, TX, GND** pinlerine bağlı olmalı.
- **Seri ayarlar:** 115200 baud, 8 data bit, 1 stop bit, parity yok (8N1). Bu ayar hem bootloader hem Python script ile aynı olmalı.

### 1.2 Yazılım

- **STM32CubeIDE** – Bootloader ve uygulama kodunu derlemek, kartı programlamak için.
- **Python 3** – Yükleme script’ini çalıştırmak için (Windows’ta python.org veya Microsoft Store’dan).
- **Seri monitör** (isteğe bağlı): PuTTY, Tera Term veya Arduino IDE Seri Port. Port: **115200**.

---

## Bölüm 2: Bootloader Dosyalarını Projeye Ekleme (bootloader.h ve bootloader.c)

Bootloader’ın çalışması için **bootloader.h** (başlık dosyası) ve **bootloader.c** (kaynak dosya) projede olmalı ve derleme sırasında kullanılmalı. Bu bölümde **neyi, nereye ve ne zaman** ekleyeceğinizi adım adım anlatıyorum.

### 2.1 Bu Dosyalar Nerede Durmalı?

- **bootloader.h** → Proje içinde **Core/Inc** klasöründe olmalı.  
  Tam yol örneği: `I2CDENEME3/Core/Inc/bootloader.h`  
  **Neden burada?** C/C++ derleyicisi projede genelde **Core/Inc** klasörünü “include path” olarak görür; `#include "bootloader.h"` yazan her dosya (bootloader.c, main_bootloader.c) bu dosyayı bulabilir.

- **bootloader.c** → Proje içinde **Core/Src** klasöründe olmalı.  
  Tam yol örneği: `I2CDENEME3/Core/Src/bootloader.c`  
  **Neden burada?** STM32CubeIDE projelerinde kaynak dosyalar (`.c`) **Core/Src** altında toplanır; proje bu klasörü otomatik derler.

Bu projede bu iki dosya zaten **var**: `Core/Inc/bootloader.h` ve `Core/Src/bootloader.c`. Önce bunların **IDE’de proje ağacında görünüp görünmediğini** kontrol etmeniz yeterli.

### 2.2 STM32CubeIDE’de Dosyaların Projede Görünmesi

1. **STM32CubeIDE**’yi açın ve **I2CDENEME3** projesini açın (Project Explorer’da projeye çift tıklayın veya sağ tık → Open).
2. Sol taraftaki **Project Explorer** penceresinde proje ağacını açın:
   - **I2CDENEME3** (en üstteki proje adı)
     - **Core**
       - **Inc** → Bu klasörün içinde **bootloader.h** listelenmeli.
       - **Src** → Bu klasörün içinde **bootloader.c** listelenmeli.

**Eğer Core/Inc veya Core/Src altında bootloader.h / bootloader.c görünmüyorsa** aşağıdaki adımlarla ekleyin.

### 2.3 bootloader.h ve bootloader.c Dosyaları Görünmüyorsa – Nasıl Eklenir?

**Senaryo:** Dosyalar diskte **Core/Inc** ve **Core/Src** içinde var ama Project Explorer’da yok (örneğin projeye sonradan kopyaladınız).

**bootloader.h ekleme:**

1. Project Explorer’da **Core** klasörünü genişletin, **Inc** klasörüne **sağ tıklayın**.
2. **Import…** (İçe Aktar) seçin.
3. **General → File System** → **Next**.
4. **Browse…** ile proje klasörünüze gidin: `I2CDENEME3/Core/Inc`.
5. **bootloader.h** kutusunu işaretleyin.
6. **Finish** deyin. **Inc** altında **bootloader.h** görünmeli.

**bootloader.c ekleme:**

1. Project Explorer’da **Core → Src** klasörüne **sağ tıklayın**.
2. **Import…** → **General → File System** → **Next**.
3. **Browse…** ile `I2CDENEME3/Core/Src` klasörünü seçin.
4. **bootloader.c** kutusunu işaretleyin.
5. **Finish** deyin. **Src** altında **bootloader.c** görünmeli.

**Neden Import?** STM32CubeIDE sadece “projeye dahil edilmiş” dosyaları derler. Diskte olsa bile proje ağacında yoksa derleme listesine alınmaz. Import ile dosyayı projeye **bağlamış** olursunuz (kopyalamak zorunda değilsiniz).

### 2.4 main_bootloader.c Dosyası

Bootloader build’de **main()** fonksiyonu **main_bootloader.c** içinde olacak (Bölüm 4’te ayarlayacağız). Bu dosya da **Core/Src** altında olmalı ve proje ağacında görünmeli: **Core/Src/main_bootloader.c**.

- Görünmüyorsa, **main_bootloader.c** için de **Core/Src**’e **Import → File System** ile ekleyin (yukarıdaki bootloader.c gibi).

### 2.5 Include Path (bootloader.h’ın Bulunması)

**bootloader.c** ve **main_bootloader.c** içinde `#include "bootloader.h"` var. Derleyicinin bu dosyayı bulması için **Core/Inc** klasörü “include path”te olmalı.

- STM32CubeIDE’de CubeMX ile oluşturulan projelerde **Core/Inc** zaten include path’te tanımlıdır. **Ekstra bir şey yapmanız gerekmez**; bootloader.h’ı Core/Inc’e koyduğunuzda derleme otomatik bulur.
- Eğer “bootloader.h: No such file or directory” hatası alırsanız: **Project → Properties → C/C++ General → Paths and Symbols → Includes** sekmesinde **GNU C** altında **Core/Inc** (veya `${workspace_loc:/${ProjName}/Core/Inc}`) yolunun olduğundan emin olun.

### 2.6 Ne Zaman Eklenecek? – Sıra Özeti

| Sıra | Ne yapıyorsunuz? |
|------|-------------------|
| 1 | bootloader.h → **Core/Inc** içinde olmalı ve Project Explorer’da **Core/Inc** altında görünmeli. |
| 2 | bootloader.c → **Core/Src** içinde olmalı ve Project Explorer’da **Core/Src** altında görünmeli. |
| 3 | main_bootloader.c → **Core/Src** içinde olmalı ve Project Explorer’da görünmeli. |
| 4 | Sonra **Bootloader** build konfigürasyonunu kullanıp linker script ve main seçimini yapacaksınız (Bölüm 4). |

Yani: **Bootloader’ı ilk kez derlemeden önce** bu üç dosyanın projede olduğundan emin olun. Bir kez eklendikten sonra hem Debug/Release hem Bootloader build’lerinde bu dosyalar projede kalır; sadece “hangi main ve hangi linker script kullanılsın”ı build konfigürasyonunda seçeceğiz.

---

## Bölüm 3: COM Port Numarasını Bulma (Windows)

Python script’i ve seri monitör **hangi COM port**’u kullanacağını bilmeli. Bu numara kartı taktığınızda Windows’un atadığı numaradır.

1. Kartı **USB** ile bilgisayara takın.
2. **Windows tuşu + X** → **Cihaz Yöneticisi**’ni açın.
3. **“Bağlantı noktaları (COM ve LPT)”** (İngilizce: “Ports (COM & LPT)”) dalını genişletin.
4. Listede **“STMicroelectronics STLink Virtual COM Port (COMx)”** veya **“USB Serial Port (COMx)”** gibi bir satır görün. **COMx** kısmındaki sayıyı (örn. COM3, COM7) bir yere not alın.
5. Aşağıdaki tüm komutlarda **`-p COMx`** yazacağınız yerde bu numarayı kullanacaksınız (örn. COM7 ise `-p COM7`).

---

## Bölüm 4: Bootloader’ı Derleyip 0x08000000’e Yükleme

Bootloader’ın kartta **ilk çalışan kod** olması için, **ayrı bir build konfigürasyonu** ile derlenip **0x08000000** adresine yüklenmesi gerekir. Bu bölümde bu konfigürasyonu nasıl kuracağınızı ve derleyip yükleyeceğinizi adım adım anlatıyorum.

### 4.1 Bootloader Build Konfigürasyonunu Açma veya Oluşturma

**Neden ayrı konfigürasyon?** Normal “Debug” build’de kod **0x08008000**’den başlayacak şekilde linklenir (uygulama için). Bootloader ise **0x08000000**’de ve sadece **32 KB** kullanacak. Aynı projede iki farklı başlangıç adresi ve iki farklı “main” kullanmak için iki ayrı **Build Configuration** kullanırız.

1. **STM32CubeIDE**’de **I2CDENEME3** projesi açık olsun.
2. Üst menüden **Project** → **Build Configurations** → **Set Active** → **Bootloader** seçin.  
   - Eğer listede **Bootloader** yoksa:
     - **Project** → **Build Configurations** → **Manage Configurations…**
     - **New** butonuna tıklayın.
     - **Name:** `Bootloader` yazın → **OK**.
     - Listeden **Bootloader**’ı seçin → **Set Active** → **Close**.

Bundan sonra yaptığınız “Build” işlemi **Bootloader** konfigürasyonuna göre yapılacak; Properties’te yapacağınız linker ve kaynak dosya ayarları da bu konfigürasyona ait.

### 4.2 Bootloader Build’de Linker Script Seçimi (Hangi .ld Dosyası?)

**Neden?** Linker script, programın FLASH’ta **nerede** ve **ne kadar** yer kaplayacağını söyler. Bootloader için FLASH **0x08000000**’den başlamalı ve **32 KB** ile sınırlı olmalı. Bu projede bunu **STM32F446RETX_BOOT.ld** dosyası yapıyor.

1. **Project Explorer**’da **I2CDENEME3** projesine **sağ tıklayın**.
2. **Properties** seçin (en altta).
3. Açılan pencerede sol tarafta **C/C++ Build** satırına tıklayın (genişletmenize gerek yok).
4. Sağ tarafta **Configuration:** açılır listesinden **Bootloader**’ı seçin. (Önemli: Aşağıdaki ayarlar sadece **Bootloader** için geçerli olsun.)
5. **C/C++ Build** → **Settings** satırına tıklayın (veya **Settings…** butonuna basın).
6. Açılan pencerede üstten **Tool Settings** sekmesine geçin.
7. Sol taraftaki ağaçta **MCU GCC Linker** → **General** bölümüne tıklayın.
8. Sağ tarafta **Linker script file (-T)** veya **Script files (-T)** kısmını bulun.
   - **Remove** ile şu anki script’i kaldırın (varsa).
   - **Add…** (veya **Browse…**) ile proje kökündeki **STM32F446RETX_BOOT.ld** dosyasını seçin.  
     Proje kökü: `I2CDENEME3` klasörünün içi (Debug ile aynı seviyede). Dosya adı: **STM32F446RETX_BOOT.ld**.
9. **Apply and Close** (veya **Apply**) deyin.

Böylece Bootloader build’de kod **0x08000000**, **32 KB** için linklenecek.

### 4.3 Bootloader Build’de Hangi main Kullanılacak? (main.c / main_bootloader.c)

**Neden?** Bir C projesinde yalnızca **bir tane** `main()` fonksiyonu olmalı. Projede hem **main.c** (uygulama için) hem **main_bootloader.c** (bootloader için) var. Bootloader build’de **sadece main_bootloader.c** derlenmeli, **main.c** bu build’de devre dışı bırakılmalı.

1. **Project Explorer**’da **Core** → **Src** klasörünü genişletin.
2. **main.c** dosyasına **sağ tıklayın** → **Resource Configurations** → **Exclude from Build…** (veya **Properties** → **Resource** → **Exclude from Build**).
3. Açılan pencerede **Bootloader** konfigürasyonunun yanındaki kutu **işaretli** olsun (sadece Bootloader’da main.c hariç tutulsun). **Debug** ve **Release** işaretli **olmasın** (normal build’de main.c derlensin).
4. **OK** deyin.
5. **main_bootloader.c** için: **Resource Configurations** → **Exclude from Build…** açın. **Sadece Debug ve Release** işaretli olsun (yani Bootloader’da **işaretsiz** olsun). Böylece Bootloader build’de sadece main_bootloader.c derlenir.

**Özet:** Bootloader build’de → main.c **exclude**, main_bootloader.c **include**. Debug/Release’te → main.c **include**, main_bootloader.c **exclude** (isterseniz; veya her ikisini de include bırakıp sadece main.c’yi Bootloader’da exclude etmek yeterli, çünkü iki main aynı anda linklenmez).

### 4.4 Bootloader’ı Derleme

1. Üst araç çubuğunda (veya Project Explorer üstünde) **Active build configuration** açılır listesinden **Bootloader** seçili olsun.
2. **Project** → **Build Project** (veya **Ctrl+B**).
3. Alttaki **Console** penceresinde derleme çıktısını izleyin. Son satırda **"Build Finished"** veya **"0 errors"** görmelisiniz.
4. Hata varsa: **bootloader.h** veya **bootloader.c** bulunamıyorsa Bölüm 2’yi tekrar kontrol edin; **multiple definition of main** benzeri hata varsa 4.3’te main.c’nin Bootloader’da exclude olduğundan emin olun.

**Bootloader build çıktısı nerede?** Bootloader konfigürasyonu ile derlediğinizde .elf/.hex/.bin **Debug** klasöründe değil, **Bootloader** klasöründe (veya proje adına göre, örn. **bootloader2.hex** proje kökünde) olabilir. Derleme bitince **Console**’daki son satırlarda çıktı yolunu görebilirsiniz; veya **Project Explorer**’da projenizi genişletin — **Bootloader** (veya benzeri) klasörünün içinde **.elf**, **.hex**, **.bin** dosyalarını arayın. Bu dosyayı kartın **0x08000000** adresine yükleyeceksiniz.

### 4.5 Bootloader .bin / .hex’i 0x08000000 Adresine Yükleme (STM32CubeIDE Run)

1. Üst menüden **Run** → **Run Configurations…** seçin.
2. Sol tarafta **STM32 C/C++ Application** dalını genişletin. Altında projenize ait bir konfigürasyon seçin (yoksa **New Configuration** ile oluşturun).
3. **Main** sekmesinde:
   - **C/C++ Application:** **Browse…** ile **Bootloader** build’in çıktı **.elf** dosyasını seçin. Çıktı **Debug**’da değilse **Bootloader** klasöründe veya proje kökünde olabilir (örn. `Bootloader/bootloader2.elf` veya `bootloader2.elf`).
4. **Apply** → **Run** deyin. IDE programı kartına yükler; ST-Link varsayılan olarak **0x08000000**’den yükler.

Bu adımdan sonra kartta **sadece bootloader** çalışır; uygulama bölgesi (0x08008000 ve sonrası) boş veya eski kalabilir.

### 4.6 Alternatif: STM32CubeProgrammer ile Yükleme

IDE’nin Run’ı yerine doğrudan **.bin** veya **.hex** dosyasıyla yüklemek isterseniz:

1. **STM32CubeProgrammer** uygulamasını açın.
2. Kartı ST-Link ile bağlayın → **Connect**.
3. **Download** bölümünde **Open file** ile Bootloader build’den çıkan **.bin** veya **.hex** dosyasını seçin. Dosya **Debug**’da değilse **Bootloader** klasöründe veya proje kökünde arayın (örn. `Bootloader/bootloader2.hex` veya `bootloader2.bin`).
4. **Start address:** **0x08000000** yazın (.hex seçtiyseniz adres genelde dosyadan okunur).
5. **Download** butonuna basın. İndirme bitince kartı **Reset** edin.

---

## Bölüm 5: Uygulama (Ana Proje) Build’i – 0x08008000

Yükleyeceğiniz firmware (Python script ile göndereceğiniz .hex/.bin) **0x08008000** adresinden başlamalı. Bu bölümde uygulama build’inin bu adrese göre ayarlı olduğunu kontrol ediyoruz.

### 5.1 Uygulama Linker Script ve VTOR

1. **Active build configuration**’ı **Debug** (veya normal kullandığınız uygulama konfigürasyonu) yapın.
2. **Project** → **Properties** → **C/C++ Build** → **Settings** → **Tool Settings** → **MCU GCC Linker** → **General**.
3. **Linker script** olarak **STM32F446RETX_FLASH.ld** (veya projenizde uygulama için kullandığınız .ld) seçili olmalı. Bu dosyanın içinde **FLASH** için **ORIGIN = 0x08008000** (ve LENGTH örn. 480K) yazıyor olmalı. Gerekirse dosyayı açıp kontrol edin.
4. Uygulama **main.c** (veya sistem başlangıcında çalışan bir dosya) içinde **SCB->VTOR = 0x08008000;** satırı olmalı. Bu, işlemcinin kesme vektör tablosunu 0x08008000’den okumasını sağlar. Yoksa main’in en başına ekleyin.

### 5.2 Uygulama Derleme ve .hex/.bin Yolu (Python ile yüklenecek dosya)

1. **Project** → **Build Project** ile **uygulama** build’ini derleyin (Active configuration: **Debug** veya uygulama konfigürasyonunuz).
2. **.hex** veya **.bin** dosyasının yerini bulun. Bu dosya **Python script ile UART’tan 0x08008000’e yükleyeceğiniz** uygulama firmware’idir.
   - Çıktı genelde **Debug/** altında olur: **Debug/I2CDENEME3.hex**, **Debug/I2CDENEME3.bin**.
   - Sizin projenizde hex **Bootloader** klasöründe veya bootloader projesinin içindeyse o yolu kullanın (örn. **Bootloader/I2CDENEME3.hex** veya **../Bootloader/bootloader2.hex**). Derleme sonrası Project Explorer’da ilgili klasöre bakarak tam yolu tespit edin.
3. Bu yolu aşağıdaki Python komutunda **firmware dosyası** argümanı olarak yazacaksınız.

---

## Bölüm 6: Python Ortamı ve Script Klasörü

1. **Komut İstemi (cmd)** veya **PowerShell** açın.
2. Proje **tools** klasörüne gidin:
   ```bat
   cd C:\Users\PC_2255\Desktop\deneme\I2CDENEME3\tools
   ```
3. Bağımlılığı yükleyin:
   ```bat
   pip install -r requirements.txt
   ```
   veya doğrudan:
   ```bat
   pip install pyserial
   ```
4. `upload_firmware.py` dosyasının bu klasörde olduğunu kontrol edin:
   ```bat
   dir upload_firmware.py
   ```

---

## Bölüm 7: Test 1 – Seri Monitörde “BOOTLOADER READY” Görmek

**Amaç:** Bootloader’ın çalıştığını ve UART’ta komut beklediğini seri monitörde görmek.

### 7.1 Kartın Yükleme Modunda Olması

- **Uygulama bölgesi boşsa:** Reset atınca bootloader çalışır, uygulama bulamaz, **yükleme moduna** girer ve UART’a **BOOTLOADER READY** yazar.
- **Uygulama bölgesi doluysa:** Reset atınca bootloader doğrudan uygulamaya atlar; yükleme moduna girmez. Bu durumda **B1 (mavi kullanıcı butonu)** basılı tutup **reset** atın; bootloader B1’i gördüğü için yükleme modunda kalır.

### 7.2 Seri Monitörü Açma

1. **PuTTY** veya **Tera Term** açın.
2. Bağlantı türü: **Serial** (Seri).
3. **Serial line:** Bölüm 3’te not ettiğiniz **COMx** (örn. COM3).
4. **Speed:** **115200**.
5. **Open** (Bağlan).

### 7.3 Reset ve Mesajı Görmek

1. Kartı **reset**leyin (Reset butonu veya gücü kesip açın).
2. Birkaç saniye içinde ekranda **BOOTLOADER READY** yazısını görmelisiniz.
3. Gördüyseniz bootloader doğru çalışıyor. Seri monitörü **kapatın** (portu serbest bırakmak için); Python script aynı portu kullanacak.

---

## Bölüm 8: Test 2 – İlk Kez Firmware Yükleme (Dosya Yok)

**Amaç:** Script ile .hex/.bin yükleyip “Yükleme tamamlandı” ve “[Cihaz] OK” görmek.

### 8.1 Kartı Yükleme Moduna Getirme

Uygulama bölgesi **boşsa** sadece **reset** atın. **Doluysa** **B1 basılı + reset** atın. Seri monitör kapalı olsun.

### 8.2 Yükleme Komutunu Çalıştırma

1. **cmd** veya **PowerShell**’de:
   ```bat
   cd C:\Users\PC_2255\Desktop\deneme\I2CDENEME3\tools
   ```
2. Aşağıdaki komutu **kendi COM port** ve **kendi .hex/.bin yolu** ile yazın. Hex dosyanız **Debug**’da değil **Bootloader** klasöründeyse o yolu verin.
   **Örnek – hex Debug’da:**
   ```bat
   python upload_firmware.py -p COM3 ..\Debug\I2CDENEME3.hex
   ```
   **Örnek – hex Bootloader klasöründe (tools klasöründen):**
   ```bat
   python upload_firmware.py -p COM3 ..\Bootloader\I2CDENEME3.hex
   ```
   **Örnek – .bin ve COM7:**
   ```bat
   python upload_firmware.py -p COM7 ..\Bootloader\bootloader2.bin
   ```
   COM port ve dosya yolunu kendi proje yapınıza göre değiştirin.

### 8.3 Ekranda Görecekleriniz

- Script önce hash kontrolü (CMD_CHECK) gönderir. Cihazda bu imaj yoksa **[Cihaz] NEED_LOAD**, sonra **Yükleme başlıyor…**, ilerleme satırları, **[Cihaz] OK** ve **Yükleme tamamlandı.** görünür.
- Ardından bootloader uygulamaya zıplar; LED vb. uygulama davranışı görülür.

---

## Bölüm 9: Test 3 – “Dosya Zaten Yüklü” (Yükleme Atlanıyor)

**Amaç:** Aynı firmware zaten varken script’i tekrar çalıştırınca “[Cihaz] ALREADY” ve “Firmware zaten yüklü, yükleme atlanıyor” görmek.

### 9.1 Bootloader’ı Yükleme Modunda Bırakmak (B1)

1. **B1 (mavi kullanıcı butonu)** basılı tutun.
2. Kartı **reset**leyin.
3. B1’i bırakın. Bootloader yükleme modunda kalır ve UART’ta komut bekler.

### 9.2 Aynı Dosyayla Script’i Tekrar Çalıştırma

Portu kullanan başka program kapalı olsun. Aynı komutu tekrar verin; **hex/bin yolunu kendi konumunuza göre** yazın (Debug veya Bootloader klasörü):

```bat
python upload_firmware.py -p COM3 ..\Bootloader\I2CDENEME3.hex
```
(Örnek: hex **Bootloader** klasöründeyse yukarıdaki gibi; **Debug**’daysa `..\Debug\I2CDENEME3.hex` yazın.)

### 9.3 Ekranda Görecekleriniz

- **[Cihaz] ALREADY** ve **Firmware zaten yüklü (MD5 eşleşti), yükleme atlanıyor.** Yeni yükleme yapılmaz.

---

## Bölüm 10: Kısa Komut Özeti

**COMx** ve **firmware yolu**nu kendi ortamınıza göre değiştirin. Hex/bin dosyanız **Debug**’da değil **Bootloader** klasöründeyse `..\Bootloader\...` kullanın.

```bat
cd C:\Users\PC_2255\Desktop\deneme\I2CDENEME3\tools
pip install -r requirements.txt

REM İlk yükleme (cihaz yükleme modunda) – hex Bootloader klasöründeyse:
python upload_firmware.py -p COMx ..\Bootloader\I2CDENEME3.hex

REM Veya hex Debug’daysa:
REM python upload_firmware.py -p COMx ..\Debug\I2CDENEME3.hex

REM Zaten yüklü testi: B1 basılı + reset, sonra aynı komut
python upload_firmware.py -p COMx ..\Bootloader\I2CDENEME3.hex
```

---

## Bölüm 11: Sık Karşılaşılan Sorunlar

| Sorun | Olası sebep | Ne yapmalı? |
|--------|-------------|-------------|
| Seri monitörde hiçbir şey yok | Yanlış COM port veya bootloader uygulamaya atladı | COM portunu Cihaz Yöneticisi’nden kontrol edin. Uygulama yüklüyse B1 basılı + reset deneyin. |
| “Port açılamadı” | Port başka programda (CubeIDE, PuTTY) açık | Seri monitörü ve diğer seri kullanan programları kapatın. |
| “Cihaz ACK vermedi” | Cihaz yükleme modunda değil veya baud uyumsuz | B1 basılı + reset ile yükleme moduna alın. 115200 kullandığınızdan emin olun. |
| “Firmware zaten yüklü” göremiyorum | Bootloader hep uygulamaya atlıyor | B1’e basılı tutup reset atın; bootloader yükleme modunda kalır, aynı komutu tekrar çalıştırın. |
| Yükleme sonrası cihaz tepkisiz | Uygulama 0x08008000 için derlenmemiş / VTOR yanlış | Linker’da FLASH ORIGIN = 0x08008000 ve main’de SCB->VTOR = 0x08008000 kontrol edin. |
| bootloader.h: No such file or directory | Core/Inc include path’te değil veya dosya projede yok | Bölüm 2’yi uygulayın; Core/Inc’te bootloader.h olduğundan ve Project Explorer’da göründüğünden emin olun. |
| multiple definition of `main' | Hem main.c hem main_bootloader.c Bootloader build’de derleniyor | Bölüm 4.3: Bootloader build’de main.c’yi “Exclude from Build” yapın. |

---

## Özet Tablo

| Bölüm | Ne yapıyorsunuz? | Başarı ne demek? |
|--------|-------------------|-------------------|
| 2 | bootloader.h, bootloader.c, main_bootloader.c projede | Project Explorer’da Core/Inc ve Core/Src altında bu dosyalar görünüyor. |
| 3 | COM port bulma | Cihaz Yöneticisi’nde COMx görünüyor. |
| 4 | Bootloader build + linker + main ayarı + 0x08000000’e yükleme | Build hatasız; Run veya CubeProgrammer ile yükleme tamamlandı. |
| 5 | Uygulama 0x08008000 build | .hex/.bin çıktısı var, linker ve VTOR 0x08008000. |
| 6 | Python + pyserial | pip install hatasız. |
| 7 | Seri monitör + reset | “BOOTLOADER READY” görünüyor. |
| 8 | İlk yükleme (script) | “[Cihaz] NEED_LOAD”, “Yükleme tamamlandı”, “[Cihaz] OK” görünüyor. |
| 9 | B1 + reset, aynı dosya ile script | “[Cihaz] ALREADY”, “Firmware zaten yüklü, yükleme atlanıyor.” görünüyor. |

Bu rehberi sırayla takip ederek bootloader dosyalarını doğru yere ekleyip, Bootloader build’i kurup, testleri yapabilirsiniz.
