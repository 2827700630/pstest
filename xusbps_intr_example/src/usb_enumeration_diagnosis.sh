#!/bin/bash
# USB枚举状态诊断脚本
# 帮助诊断ZYNQ7010 USB CDC设备枚举问题

echo "=========================================="
echo "USB枚举状态诊断工具"
echo "目标: 诊断设备描述符请求失败问题"
echo "=========================================="

# 定义颜色
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}关键修复点检查:${NC}"
echo "1. 设备描述符结构:"
echo "   - bLength: 18 bytes"
echo "   - bDeviceClass: 0x02 (CDC)" 
echo "   - idVendor: 0x04B4 (Cypress)"
echo "   - idProduct: 0x0008"
echo "   - bMaxPacketSize0: 64 bytes"
echo ""

echo "2. 配置描述符结构:"
echo "   - wTotalLength: 完整结构大小"
echo "   - bNumInterfaces: 2 (Control + Data)"
echo "   - bmAttributes: 0x80 (Bus Powered)"
echo "   - bMaxPower: 0x32 (100mA)"
echo ""

echo "3. 端点配置:"
echo "   - EP0: Control, 64 bytes"
echo "   - EP1: Bulk Data, 64 bytes (兼容模式)"
echo "   - EP2: Interrupt Notification, 16 bytes"
echo ""

echo -e "${YELLOW}预期UART调试输出序列:${NC}"
echo "1. 设备启动:"
echo "   ${GREEN}ZYNQ7010 USB CDC-ACM Virtual Serial Port - Deep Compatibility Mode${NC}"
echo "   ${GREEN}Device Class: CDC (0x02) - Maximum Compatibility${NC}"
echo ""

echo "2. USB连接后:"
echo "   ${GREEN}检测到USB复位${NC}"
echo "   ${GREEN}USB Setup: Type=0x80, Req=0x06, Val=0x0100, Idx=0x0000, Len=18${NC}"
echo "   ${GREEN}Device Descriptor Request: BufLen=18, DescSize=18${NC}"
echo "   ${GREEN}Device Descriptor sent successfully: 18 bytes${NC}"
echo ""

echo "3. 枚举完成:"
echo "   ${GREEN}USB枚举完成，设备已连接${NC}"
echo ""

echo -e "${RED}问题征象:${NC}"
echo "• 如果看到 'Device Descriptor Request' 但没有 'sent successfully'"
echo "  → 描述符构建有问题"
echo ""
echo "• 如果完全没有 'USB Setup' 消息"
echo "  → USB物理连接或PHY配置问题"
echo ""
echo "• 如果有Setup消息但Windows仍报错"
echo "  → 描述符内容不符合Windows驱动要求"
echo ""

echo -e "${YELLOW}分阶段诊断方法:${NC}"
echo "阶段1: 硬件连接验证"
echo "  - 检查USB线缆"
echo "  - 确认ZYNQ7010供电正常"
echo "  - 验证USB3320C PHY供电(1.8V)"
echo ""

echo "阶段2: 软件枚举验证"
echo "  - 监控UART输出是否有USB Setup消息"
echo "  - 确认设备描述符请求和响应"
echo "  - 检查配置描述符交互"
echo ""

echo "阶段3: 主机端验证"
echo "  - Windows设备管理器状态"
echo "  - 尝试不同的USB端口"
echo "  - 使用USB设备查看工具"
echo ""

echo -e "${YELLOW}故障排除命令:${NC}"
echo "Windows:"
echo "  # 查看USB设备详情"
echo "  # devmgmt.msc (设备管理器)"
echo "  # 或下载USBView工具"
echo ""

echo "Linux:"
echo "  sudo dmesg -w    # 实时监控USB消息"
echo "  lsusb -v | grep -A20 '04b4:0008'"
echo "  cat /proc/bus/usb/devices | grep -A10 'Vendor=04b4'"
echo ""

echo -e "${GREEN}成功标志:${NC}"
echo "✓ UART显示: Device Descriptor sent successfully"
echo "✓ UART显示: USB枚举完成，设备已连接"
echo "✓ Windows设备管理器显示CDC设备"
echo "✓ 设备驱动程序正常加载"
echo ""

echo "=========================================="
echo "诊断脚本准备完成"
echo "请编译固件，连接设备并观察输出"
echo "=========================================="
