# ZYNQ7010 USB CDC-ACM 虚拟串口设备

本项目为 ZYNQ7010 SoC 实现了一个 USB CDC-ACM（通信设备类 - 抽象控制模型）虚拟串口设备，该设备在主机 PC 上显示为标准的 COM 端口。

## 项目特点

- **标准兼容性**: 使用 USB CDC-ACM 标准，无需安装特殊驱动程序
- **跨平台支持**: 兼容 Windows、Linux 和 macOS
- **虚拟串口**: 在主机上显示为标准 COM 端口（Windows）或 /dev/ttyACMx（Linux）
- **数据回显**: 接收到的数据会以 "[Echo #N]" 前缀回显给主机
- **解决 PyUSB 问题**: 通过使用标准 CDC 驱动程序避免 PyUSB NoBackendError

## 硬件要求

- ZYNQ7010 开发板
- USB 连接器（设备端口）
- USB 数据线
- 串口调试终端（可选，用于查看调试信息）

## 软件要求

### 开发环境
- Xilinx Vitis 2023.2 或更高版本
- 兼容的 ZYNQ7010 硬件平台

### 测试环境
- Python 3.6+ 
- PySerial 库: `pip install pyserial`
- 或任何串口终端软件（PuTTY、TeraTerm、串口调试助手等）

## 快速开始

### 1. 构建项目

1. **打开 Vitis IDE**
   ```
   启动 Xilinx Vitis 2023.2
   ```

2. **导入项目**
   - 选择 File → Import → Existing Projects into Workspace
   - 浏览到项目目录并导入

3. **选择目标设备**
   - 确保选择了正确的 ZYNQ7010 硬件平台
   - 验证 USB 控制器配置

4. **构建应用程序**
   ```
   右键点击项目 → Build Project (Ctrl+B)
   ```

5. **生成和烧录比特流**（如果需要）
   - 生成硬件比特流文件
   - 将比特流烧录到 FPGA

### 2. 硬件连接

1. **连接 USB 线缆**
   - 将 USB 线缆连接到 ZYNQ 板的 USB 设备端口
   - 另一端连接到 PC

2. **上电**
   - 确保 ZYNQ 板已正确上电
   - 检查电源指示灯

3. **烧录程序**
   - 将编译好的应用程序下载到 ZYNQ7010
   - 程序会自动启动

### 3. 验证连接

1. **检查设备枚举**
   - **Windows**: 打开设备管理器，查看"端口(COM 和 LPT)"
   - **Linux**: 运行 `dmesg | grep ttyACM` 或 `ls /dev/ttyACM*`
   - **macOS**: 运行 `ls /dev/tty.usbmodem*`

2. **查看设备信息**
   - 设备名称: "ZYNQ7010 CDC Virtual Serial Port"
   - 制造商: "Xilinx"
   - 供应商 ID: 0x0d7d
   - 产品 ID: 0x1234

### 4. 测试通信

#### 方法一：使用 Python 测试脚本（推荐）

```bash
# 安装依赖
pip install pyserial

# 运行测试脚本（自动检测端口）
python test_usb_device.py

# 或手动指定端口
python test_usb_device.py COM3        # Windows
python test_usb_device.py /dev/ttyACM0 # Linux
```

#### 方法二：使用串口终端

1. **打开串口终端软件**
   - Windows: PuTTY、TeraTerm、串口调试助手
   - Linux: minicom、screen、picocom
   - macOS: screen、CoolTerm

2. **配置连接参数**
   - 端口: 检测到的 COM 端口或 /dev/ttyACMx
   - 波特率: 115200（任意值，CDC 设备会忽略）
   - 数据位: 8
   - 停止位: 1
   - 校验位: 无

3. **测试通信**
   - 输入任意文本并按回车
   - 应该收到 "[Echo #N] 您输入的文本" 格式的回显

## 技术实现

### USB 描述符配置

- **设备类**: 0xEF (杂项设备)
- **设备子类**: 0x02 (通用类)
- **设备协议**: 0x01 (IAD - 接口关联描述符)
- **接口**: 2个（控制接口 + 数据接口）
- **端点**: 3个
  - EP0: 控制端点（双向）
  - EP1: 数据端点（批量传输，双向）
  - EP2: 通知端点（中断传输，IN）

### CDC 功能描述符

- **头功能描述符**: 标识 CDC 版本
- **ACM 功能描述符**: 抽象控制管理能力
- **联合功能描述符**: 关联控制和数据接口
- **调用管理功能描述符**: 调用管理能力

### 端点配置

```c
// 端点 0 - 控制端点
DeviceConfig.EpCfg[0].Out.Type = XUSBPS_EP_TYPE_CONTROL;
DeviceConfig.EpCfg[0].Out.MaxPacketSize = 64;

// 端点 1 - CDC 数据端点（批量传输）
DeviceConfig.EpCfg[1].Out.Type = XUSBPS_EP_TYPE_BULK;
DeviceConfig.EpCfg[1].Out.MaxPacketSize = 512;
DeviceConfig.EpCfg[1].In.Type = XUSBPS_EP_TYPE_BULK;
DeviceConfig.EpCfg[1].In.MaxPacketSize = 512;

// 端点 2 - CDC 通知端点（中断传输）
DeviceConfig.EpCfg[2].In.Type = XUSBPS_EP_TYPE_INTERRUPT;
DeviceConfig.EpCfg[2].In.MaxPacketSize = 16;
```

## 项目文件结构

```
src/
├── xusbps_intr_example.c      # 主应用程序
├── xusbps_ch9.c/h             # USB 第9章标准请求处理
├── xusbps_ch9_storage.c/h     # USB 描述符定义
├── xusbps_class_cdc.c/h       # CDC 类特定请求处理
├── test_usb_device.py         # Python 测试脚本
├── README.md                  # 英文项目说明文档
├── README_CN.md               # 中文项目说明文档（本文件）
├── CMakeLists.txt             # CMake 构建配置
├── lscript.ld                 # 链接脚本
├── backup_storage_class/      # 备份的旧存储类文件
│   ├── xusbps_class_storage.c
│   └── xusbps_class_storage.h
└── backup_docs/               # 备份的旧文档文件
    ├── README_zh.md
    ├── BUILD_INSTRUCTIONS.md
    └── BUILD_INSTRUCTIONS_zh.md
```

### 主要修改文件

1. **xusbps_intr_example.c**
   - 修改为支持 CDC-ACM 操作
   - 添加回显功能和调试输出
   - 配置 3 个端点（控制、数据、通知）

2. **xusbps_ch9_storage.c**
   - 更新设备描述符为 CDC 兼容格式
   - 添加 IAD 和完整的 CDC 配置描述符
   - 修改产品字符串

3. **xusbps_class_cdc.c/h**（新文件）
   - 实现 CDC 类特定请求处理
   - 支持 SET_LINE_CODING、GET_LINE_CODING、SET_CONTROL_LINE_STATE
   - 替换原有的存储类处理器

4. **test_usb_device.py**
   - 从 PyUSB 改为 PySerial 实现
   - 自动检测 CDC 设备
   - 提供交互式通信模式

## 调试和故障排除

### 常见问题

#### 1. 设备未被识别

**症状**: 插入 USB 后设备管理器中没有出现 CDC 设备

**解决方案**:
- 检查 USB 线缆连接
- 确认 ZYNQ 板已正确上电
- 查看 UART 调试输出是否有错误信息
- 验证 USB 控制器硬件配置

#### 2. COM 端口不出现

**症状**: 设备被识别但没有 COM 端口

**解决方案**:
- 等待系统自动安装 CDC 驱动（通常很快）
- 刷新设备管理器
- 尝试不同的 USB 端口
- 检查是否有驱动冲突

#### 3. 无法通信

**症状**: COM 端口存在但发送数据无响应

**解决方案**:
- 确认串口终端设置正确（115200, 8-N-1）
- 检查 UART 调试输出中的数据接收信息
- 尝试发送带有换行符的数据
- 验证设备枚举是否完成

#### 4. 数据传输错误

**症状**: 接收到的数据不完整或错误

**解决方案**:
- 检查缓存对齐和 DMA 内存配置
- 监控 UART 输出中的错误报告
- 确保数据长度在缓冲区范围内
- 验证端点配置参数

### 调试输出

程序提供详细的 UART 调试输出（115200 波特率）：

```
USB CDC-ACM Virtual Serial Port Device started on ZYNQ7010
=====================================================
Waiting for USB host connection...
Device will appear as a virtual serial port (COM port) on host.
Connect USB cable and use serial terminal to communicate.

检测到USB复位
USB枚举完成，设备已连接
收到USB中断 (次数: 1, 掩码: 0x00000002)
在EP0上接收到设置数据包
在 EP1 上接收到数据
从主机接收到 12 字节数据: Hello World!
已发送回显响应: [Echo #1] Hello World!
```

### 性能监控

- **中断计数**: 监控 USB 中断频率
- **数据吞吐量**: 跟踪发送/接收的字节数
- **错误率**: 记录传输失败次数
- **延迟**: 测量响应时间

## 扩展功能

### 可能的改进

1. **多串口支持**: 实现多个虚拟串口
2. **流控制**: 添加 RTS/CTS 硬件流控制模拟
3. **波特率处理**: 根据主机设置的波特率调整行为
4. **缓冲区管理**: 实现更大的发送/接收缓冲区
5. **错误处理**: 增强错误检测和恢复机制

### 集成示例

```c
// 在应用程序中集成 CDC 功能
void application_main() {
    // 初始化 USB CDC 设备
    init_usb_cdc_device();
    
    // 主循环
    while (1) {
        // 检查是否有数据接收
        if (cdc_data_available()) {
            char buffer[256];
            int len = cdc_read_data(buffer, sizeof(buffer));
            
            // 处理接收到的数据
            process_received_data(buffer, len);
            
            // 发送响应
            cdc_send_response(response_data, response_len);
        }
        
        // 其他应用逻辑
        application_process();
    }
}
```

## 参考资料

- [USB CDC-ACM 规范](https://www.usb.org/sites/default/files/CDC1.2_WMC1.1_012011.pdf)
- [Xilinx ZYNQ-7000 技术参考手册](https://www.xilinx.com/support/documentation/user_guides/ug585-Zynq-7000-TRM.pdf)
- [USB 设备类定义](https://www.usb.org/defined-class-codes)
- [Vitis 统一软件平台文档](https://www.xilinx.com/support/documentation/sw_manuals/xilinx2023_2/ug1416-vitis-documentation.pdf)

## 版本历史

- **v2.0**: 转换为 CDC-ACM 虚拟串口设备
- **v1.0**: 原始的厂商特定 Hello World 设备

## 许可证

本项目基于 MIT 许可证开源。详细信息请参见源文件头部的版权声明。

## 贡献

欢迎提交问题报告和改进建议。请确保在提交代码前进行充分测试。

---

如有问题，请检查 UART 调试输出获取详细的错误信息，或参考上述故障排除部分。
