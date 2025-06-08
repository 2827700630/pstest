# USB故障诊断信息收集脚本
# 请按照此清单逐项检查并记录结果

Write-Host "=========================================="
Write-Host "USB故障诊断信息收集" -ForegroundColor Yellow
Write-Host "=========================================="

Write-Host ""
Write-Host "📋 请按照以下清单逐项检查:" -ForegroundColor Green

Write-Host ""
Write-Host "1. 硬件配置检查" -ForegroundColor Cyan
Write-Host "   ✓ J5跳线帽状态: [  ] 已移除  [  ] 未移除"
Write-Host "   ✓ J6跳线帽状态: [  ] 已移除  [  ] 未移除"  
Write-Host "   ✓ USB连接接口: [  ] J4(Micro USB)  [  ] J3(Type A)"
Write-Host "   ✓ USB线缆类型: [  ] 数据线  [  ] 充电线  [  ] 不确定"
Write-Host "   ✓ 电脑USB端口: [  ] USB 2.0  [  ] USB 3.0  [  ] USB Hub"

Write-Host ""
Write-Host "2. 电源和连接检查" -ForegroundColor Cyan  
Write-Host "   ✓ ZYNQ7010电源指示灯: [  ] 正常  [  ] 异常"
Write-Host "   ✓ USB连接后Windows提示音: [  ] 有  [  ] 无"
Write-Host "   ✓ 设备管理器中设备出现: [  ] 是  [  ] 否"

Write-Host ""
Write-Host "3. UART调试输出检查" -ForegroundColor Cyan
Write-Host "   请检查UART输出中是否包含以下关键信息:"
Write-Host ""
Write-Host "   启动信息 (必须有):"
Write-Host "   [  ] 'ZYNQ7010 USB CDC-ACM Virtual Serial Port'"
Write-Host "   [  ] 'Device Class: CDC (0x02)'"
Write-Host "   [  ] 'Vendor ID: 0x04B4'"
Write-Host ""
Write-Host "   USB连接检测 (插入USB时应该有):"
Write-Host "   [  ] '检测到USB复位'"
Write-Host "   [  ] '收到USB中断'"
Write-Host ""
Write-Host "   USB枚举过程 (关键!):"
Write-Host "   [  ] 'USB Setup: Type=0x80, Req=0x06...'"
Write-Host "   [  ] 'Device Descriptor Request: BufLen=18'"
Write-Host "   [  ] 'Device Descriptor sent successfully: 18 bytes'"
Write-Host "   [  ] 'Config Descriptor Request'"
Write-Host "   [  ] 'Config Descriptor sent successfully'"
Write-Host "   [  ] 'USB枚举完成，设备已连接'"

Write-Host ""
Write-Host "4. Windows系统检查" -ForegroundColor Cyan
Write-Host "   在设备管理器中:"
Write-Host "   [  ] 设备显示为'未知USB设备(设备描述符请求失败)'"
Write-Host "   [  ] 错误代码43"
Write-Host "   [  ] 设备位置: Port_#0002.Hub_#0004"

Write-Host ""
Write-Host "=========================================="
Write-Host "请根据上述检查结果提供以下信息:" -ForegroundColor Yellow
Write-Host "=========================================="

Write-Host ""
Write-Host "📤 需要提供的信息:" -ForegroundColor Green
Write-Host ""
Write-Host "1. 完整的UART调试输出"
Write-Host "   (从设备上电到插入USB线缆的完整过程)"
Write-Host ""
Write-Host "2. 硬件配置确认"
Write-Host "   - 跳线帽是否正确移除"
Write-Host "   - 使用的USB接口和线缆类型"
Write-Host ""
Write-Host "3. 错误出现的时机"
Write-Host "   - Windows何时显示设备"
Write-Host "   - 错误何时出现"
Write-Host "   - UART输出停止在哪一步"

Write-Host ""
Write-Host "🔧 立即测试步骤:" -ForegroundColor Red
Write-Host ""
Write-Host "1. 断开USB连接"
Write-Host "2. 重启开发板"  
Write-Host "3. 等待UART显示启动完成信息"
Write-Host "4. 插入USB线缆"
Write-Host "5. 立即观察UART输出变化"
Write-Host "6. 同时观察Windows设备管理器"

Write-Host ""
Write-Host "📋 常见问题快速检查:" -ForegroundColor Magenta
Write-Host ""
Write-Host "问题A: UART完全没有USB相关输出"
Write-Host "  → 检查固件是否正确烧录"
Write-Host "  → 检查跳线设置和USB接口"
Write-Host ""
Write-Host "问题B: 有'USB复位'但没有'Setup'消息"  
Write-Host "  → 更换USB线缆"
Write-Host "  → 尝试不同USB端口"
Write-Host "  → 使用USB 2.0端口"
Write-Host ""
Write-Host "问题C: 有'Setup'但没有'sent successfully'"
Write-Host "  → USB描述符问题，需要使用极简版本"
Write-Host ""

Write-Host "=========================================="
Write-Host "请收集上述信息后反馈，以便进一步诊断" -ForegroundColor Yellow
Write-Host "=========================================="
