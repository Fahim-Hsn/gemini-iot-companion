@echo off
setlocal
echo ===================================================
echo   Project Bondhu - 1-Click Firmware Flash Tool
echo ===================================================

set PIO_PY="C:\Users\%USERNAME%\.platformio\penv\Scripts\python.exe"
set ESPTOOL="C:\Users\%USERNAME%\.platformio\packages\tool-esptoolpy\esptool.py"
set PORT=COM9

if not exist %PIO_PY% (
    set PIO_PY=python
)

echo [1/3] Building latest firmware...
"C:\Users\%USERNAME%\.platformio\penv\Scripts\pio.exe" run
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed!
    pause
    exit /b %ERRORLEVEL%
)

echo [2/3] Generating flash binary...
%PIO_PY% %ESPTOOL% --chip esp32c6 elf2image --flash_mode dio --flash_freq 80m --flash_size 8MB -o .pio\build\esp32-c6-devkitc-1\firmware.bin .pio\build\esp32-c6-devkitc-1\firmware.elf

echo [3/3] Uploading to %PORT%...
%PIO_PY% %ESPTOOL% --chip esp32c6 --port %PORT% --baud 921600 write_flash 0x10000 .pio\build\esp32-c6-devkitc-1\firmware.bin

if %ERRORLEVEL% EQU 0 (
    echo ===================================================
    echo   Flash Completed Successfully! Bondhu is ready.
    echo ===================================================
) else (
    echo [ERROR] Flashing failed! Check if COM port is correct.
)
pause
