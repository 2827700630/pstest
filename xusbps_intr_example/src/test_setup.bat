@echo off
echo ZYNQ7010 USB Hello World Device - Windows Test Setup
echo ====================================================
echo.

echo Checking for Python...
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python is not installed or not in PATH
    echo Please install Python 3.x from https://python.org
    pause
    exit /b 1
)

echo Python found. Checking for PyUSB...
python -c "import usb.core" >nul 2>&1
if errorlevel 1 (
    echo PyUSB not found. Installing...
    pip install pyusb
    if errorlevel 1 (
        echo ERROR: Failed to install PyUSB
        echo Please run: pip install pyusb
        pause
        exit /b 1
    )
)

echo.
echo Setup complete! 
echo.
echo Instructions:
echo 1. Make sure your ZYNQ7010 board is connected via USB
echo 2. The device should enumerate as "ZYNQ7010 Hello World USB Device"
echo 3. Check Device Manager to verify the device is detected
echo 4. You may need to install WinUSB driver using Zadig tool
echo.
echo Press any key to run the test script...
pause >nul

echo.
echo Running USB Hello World test...
python test_usb_device.py

echo.
echo Test completed. Press any key to exit...
pause >nul
