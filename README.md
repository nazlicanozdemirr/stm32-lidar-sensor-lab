# STM32 LiDAR Sensor Lab

STM32F446RE experiments for Benewake TF02-Pro and TFmini Plus LiDAR sensors over I²C/UART, with host-side serial diagnostics and a UART bootloader integration study.

## Retained projects

- `projects/TF02PRO1/` — TF02-Pro sensor interface.
- `projects/TFMINIPLUS1/` — TFmini Plus sensor interface.
- `projects/I2CDENEME3/` — integrated LiDAR experiments and bootloader-related application code.

Earlier trial projects and duplicated vendor HAL trees were removed from the active tree. The `.ioc` files, application sources, headers, and linker scripts remain so each project can be regenerated with STM32CubeMX/STM32CubeIDE.

## Host tools

The `tools/` directory contains small serial utilities for:

- monitoring measurement rate;
- displaying parsed distance samples;
- inspecting raw serial traffic;
- collecting detailed timing diagnostics.

Install the host dependency with:

```bash
python -m venv .venv
# Windows: .venv\Scripts\activate
# Linux/macOS: source .venv/bin/activate
python -m pip install -r requirements.txt
```

Examples:

```bash
python tools/raw_serial_check.py COM10 9600
python tools/lidar_rate_monitor.py COM10
python tools/mesafe_goster.py COM10 9600
```

## Regenerating an STM32 project

1. Open the selected `.ioc` file in STM32CubeMX or STM32CubeIDE.
2. Confirm the STM32F446RE target, clock tree, I²C/UART pins, and interrupt configuration.
3. Regenerate the HAL/CMSIS support files.
4. Verify that the retained `Core/Inc`, `Core/Src`, and linker files are preserved or merged correctly.
5. Build and test first on recoverable development hardware.

## Hardware notes

Wiring, test procedures, sensor configuration, and the resistance/serial-number worksheet are retained under `docs/`. They contain bench measurements only; no credentials or access secrets are required by the code.

## Status

The repository records laboratory firmware and validation utilities. Hardware-in-the-loop behavior depends on the exact sensor revision, wiring, power, and CubeMX configuration, so review those parameters before flashing.
