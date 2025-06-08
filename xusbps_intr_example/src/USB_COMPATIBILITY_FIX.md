# USB CDC-ACM 兼容性修复和诊断指南

## 🔧 已修复的问题

根据您的"设备描述符请求失败"错误，我已经进行了以下修复：

### 1. 恢复兼容性配置
```c
// 端点大小：512字节 → 64字节 (兼容性优先)
#define CDC_DATA_EPSIZE 64

// 设备包大小：保持兼容性
DeviceConfig.EpCfg[1].Out.MaxPacketSize = 64;
DeviceConfig.EpCfg[1].In.MaxPacketSize = 64;

// 缓冲区大小：保持512字节支持高性能
DeviceConfig.EpCfg[1].Out.BufSize = 512;
```

### 2. 更新设备标识
```c
// 使用Xilinx官方VID/PID
be2les(0x03FD),  /* idVendor - Xilinx */
be2les(0x0013),  /* idProduct - ZYNQ CDC设备 */
be2les(0x0100),  /* bcdDevice - 兼容性版本 */
```

### 3. 添加调试信息
- USB设置包调试
- 设备描述符请求调试
- 错误详细信息输出

## 🔍 诊断步骤

### 1. 重新编译和测试
```bash
# 重新编译固件
# 烧录到ZYNQ7010开发板
# 重新连接USB线缆
```

### 2. 检查UART输出
连接UART调试线，查看以下信息：
```
USB CDC-ACM Virtual Serial Port Device started on ZYNQ7010
Hardware: USB3320C-EZK PHY - Auto-Negotiating Speed
Endpoint Size: 64 bytes (Compatible Mode, Auto High-Speed)
...
USB Setup: Type=0x80, Req=0x06, Val=0x0100, Idx=0x0000, Len=64
Device Descriptor Request: BufLen=64, DescSize=18
Device Descriptor sent successfully: 18 bytes
```

### 3. 常见问题排查

#### 问题1: 仍然无法识别
**可能原因：**
- USB时钟配置问题
- PHY初始化失败
- 电源问题

**解决方法：**
1. 检查24MHz晶振是否正常
2. 验证1.8V电源稳定
3. 检查USB线缆质量
4. 尝试不同的USB端口

#### 问题2: 枚举部分成功但功能异常
**UART输出检查：**
```
USB Setup: Type=0x80, Req=0x06, Val=0x0200, Idx=0x0000, Len=9  // 配置描述符
USB Setup: Type=0x21, Req=0x20, Val=0x0000, Idx=0x0000, Len=7  // CDC请求
```

#### 问题3: 速度协商问题
**检查项目：**
- USB3320C-EZK ULPI配置
- ZYNQ USB控制器时钟
- DMA内存对齐

## 🛠️ 高级调试

### 1. USB分析器调试
如果有USB协议分析器，检查：
- 设备连接时的USB重置信号
- 描述符请求和响应
- 速度协商过程

### 2. Windows设备管理器详细信息
右键设备→属性→详细信息→查看：
- 硬件ID
- 兼容ID  
- 设备实例路径
- 设备状态

### 3. 替代测试方法
```c
// 如果问题持续，可以尝试最简配置
#define CDC_DATA_EPSIZE 32    // 更小的端点
// 或者禁用某些功能描述符进行测试
```

## 🎯 预期结果

修复后应该看到：
1. Windows设备管理器中显示"USB Serial Device"或类似名称
2. 设备状态为"正常工作"
3. 出现新的COM端口
4. UART输出显示成功的USB枚举过程

## 📝 进一步优化

一旦基本枚举成功，可以逐步优化：
1. 增加端点大小到128字节
2. 测试高速模式协商
3. 优化传输性能
4. 添加高级CDC功能

---
**修复版本**: 兼容性优先版本  
**推荐**: 先确保基本功能，再逐步优化性能
