# STM32 LiDAR Sensor Lab

Bu depo; TF02-Pro, TFmini Plus ve SEN0585 LiDAR sensörleriyle yapılan STM32
I2C/UART bağlantı, veri hızı, mesafe okuma ve hata ayıklama çalışmalarını
düzenli bir deney arşivinde toplar.

## İçerik

- `projects/I2CDENEME3/`: LiDAR sürücüleri ile bootloader çalışmalarını
  birleştiren kapsamlı STM32CubeIDE projesi.
- `projects/TF02PRO1/`: TF02-Pro deney projesi.
- `projects/TFMINIPLUS1/`: TFmini Plus deney projesi.
- `projects/I2CDENEME2/`, `ilk/`, `sensordeneme/`, `sensordeneme2/` ve
  `sensordeneme3/`: önceki geliştirme ve sensör bağlantı denemeleri.
- `tools/`: seri porttan hız, ham veri ve mesafe izleme araçları.
- `reference/stm32-user-code/`: bağımsız STM32 kullanıcı kodu kopyaları.
- `docs/`: bağlantı, test, hata ayıklama ve kullanım notları.

## Python araçları

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
python .\tools\lidar_rate_monitor.py --help
```

Seri port adı, baud hızı, I2C adresi ve sensör çalışma modu kullanılan karta
göre ayarlanmalıdır. Gerçek donanımı bağlamadan önce sensörün besleme gerilimi
ve pin haritasını doğrulayın.

STM32CubeIDE projelerinde üretilen `Debug`/`Release` dosyaları ve IDE metadata
alanları depoya dahil edilmez.
