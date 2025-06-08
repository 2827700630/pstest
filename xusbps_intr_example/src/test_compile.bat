@echo off
echo 测试编译USB描述符文件...

REM 创建模拟的头文件以进行语法检查
echo Creating mock headers for syntax check...

echo #ifndef MOCK_HEADERS_H > mock_headers.h
echo #define MOCK_HEADERS_H >> mock_headers.h
echo. >> mock_headers.h
echo // 模拟 Xilinx 头文件定义 >> mock_headers.h
echo typedef unsigned char u8; >> mock_headers.h
echo typedef unsigned short u16; >> mock_headers.h
echo typedef unsigned int u32; >> mock_headers.h
echo typedef unsigned int uint32_t; >> mock_headers.h
echo typedef unsigned char uint8_t; >> mock_headers.h
echo typedef unsigned short uint16_t; >> mock_headers.h
echo. >> mock_headers.h
echo // 模拟 USB 常量 >> mock_headers.h
echo #define USB_DEVICE_DESC 0x01 >> mock_headers.h
echo #define USB_CONFIG_DESC 0x02 >> mock_headers.h
echo #define USB_STRING_DESC 0x03 >> mock_headers.h
echo #define USB_INTERFACE_CFG_DESC 0x04 >> mock_headers.h
echo #define USB_ENDPOINT_CFG_DESC 0x05 >> mock_headers.h
echo #define XUSBPS_TYPE_INTERFACE_ASSOCIATION 0x0B >> mock_headers.h
echo #define XUSBPS_TYPE_CS_INTERFACE 0x24 >> mock_headers.h
echo #define XUSBPS_CDC_FND_HEADER 0x00 >> mock_headers.h
echo #define XUSBPS_CDC_FND_CALL_MGMT 0x01 >> mock_headers.h
echo #define XUSBPS_CDC_FND_ACM 0x02 >> mock_headers.h
echo #define XUSBPS_CDC_FND_UNION 0x05 >> mock_headers.h
echo. >> mock_headers.h
echo // 模拟函数 >> mock_headers.h
echo #define be2les(x) (x) >> mock_headers.h
echo void* memcpy(void* dest, const void* src, size_t n); >> mock_headers.h
echo size_t strlen(const char* str); >> mock_headers.h
echo. >> mock_headers.h
echo #endif >> mock_headers.h

REM 创建测试编译用的简化版本
echo Creating test version...
echo #include "mock_headers.h" > test_descriptors.c
echo #include "usb_descriptors.h" >> test_descriptors.c
echo. >> test_descriptors.c
echo // 只测试描述符结构定义 >> test_descriptors.c
echo int main() { >> test_descriptors.c
echo     USB_STD_STRING_DESC test; >> test_descriptors.c
echo     USB_CDC_CONFIG_DESC_FULL config; >> test_descriptors.c
echo     return 0; >> test_descriptors.c
echo } >> test_descriptors.c

echo Checking syntax with GCC...
gcc -c test_descriptors.c -I. -Wall -Wextra 2>&1

echo.
echo Cleanup...
del mock_headers.h 2>nul
del test_descriptors.c 2>nul
del test_descriptors.o 2>nul

echo Done.
pause
