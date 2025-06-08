# Windows USB设备信息收集脚本
# 用于收集USB设备的详细信息以帮助诊断问题

Write-Host "=========================================="
Write-Host "Windows USB设备信息收集" -ForegroundColor Green
Write-Host "=========================================="

Write-Host ""
Write-Host "正在收集USB设备信息..." -ForegroundColor Yellow

# 收集USB设备信息
Write-Host ""
Write-Host "1. 当前USB设备列表:" -ForegroundColor Cyan
try {
    Get-WmiObject -Class Win32_USBHub | Select-Object Name, DeviceID, Status | Format-Table -AutoSize
} catch {
    Write-Host "无法获取USB Hub信息" -ForegroundColor Red
}

Write-Host ""
Write-Host "2. PnP设备信息 (包括问题设备):" -ForegroundColor Cyan
try {
    Get-WmiObject -Class Win32_PnPEntity | Where-Object {$_.Name -like "*USB*" -or $_.Name -like "*未知*" -or $_.Name -like "*unknown*"} | 
    Select-Object Name, DeviceID, Status, ConfigManagerErrorCode | Format-Table -AutoSize
} catch {
    Write-Host "无法获取PnP设备信息" -ForegroundColor Red
}

Write-Host ""
Write-Host "3. USB控制器信息:" -ForegroundColor Cyan
try {
    Get-WmiObject -Class Win32_USBController | Select-Object Name, Status, DeviceID | Format-Table -AutoSize
} catch {
    Write-Host "无法获取USB控制器信息" -ForegroundColor Red
}

Write-Host ""
Write-Host "4. 系统事件日志中的USB相关错误:" -ForegroundColor Cyan
try {
    Get-EventLog -LogName System -Source "USB*" -Newest 10 -ErrorAction SilentlyContinue | 
    Select-Object TimeGenerated, Source, EventID, Message | Format-Table -Wrap
} catch {
    Write-Host "无法获取USB事件日志" -ForegroundColor Red
}

Write-Host ""
Write-Host "=========================================="
Write-Host "手动检查步骤:" -ForegroundColor Yellow
Write-Host "=========================================="

Write-Host ""
Write-Host "1. 打开设备管理器检查:"
Write-Host "   - 按 Win+X, 选择 '设备管理器'"
Write-Host "   - 查看 '通用串行总线控制器' 部分"
Write-Host "   - 查看是否有黄色警告图标的设备"
Write-Host "   - 右键问题设备，选择 '属性' → '详细信息'"

Write-Host ""
Write-Host "2. 设备属性检查 (在设备管理器中):"
Write-Host "   - 设备状态: ________________"
Write-Host "   - 错误代码: ________________"  
Write-Host "   - 硬件ID: __________________"
Write-Host "   - 设备实例路径: ____________"

Write-Host ""
Write-Host "3. USB端口测试:"
Write-Host "   - 当前使用端口类型: ________"
Write-Host "   - 尝试其他USB端口结果: ____"
Write-Host "   - USB 2.0端口测试结果: ____"

Write-Host ""
Write-Host "4. 线缆和连接测试:"
Write-Host "   - 当前USB线缆型号: _________"
Write-Host "   - 更换线缆测试结果: _______"
Write-Host "   - 其他设备连接测试: _______"

Write-Host ""
Write-Host "=========================================="
Write-Host "USB设备枚举过程分析:" -ForegroundColor Magenta
Write-Host "=========================================="

Write-Host ""
Write-Host "正常的USB枚举过程应该是:"
Write-Host "1. 设备插入 → Windows检测到新设备"
Write-Host "2. 获取设备描述符 → 识别设备基本信息"  
Write-Host "3. 设置地址 → 分配USB地址"
Write-Host "4. 获取配置描述符 → 了解设备功能"
Write-Host "5. 加载驱动程序 → 设备可用"

Write-Host ""
Write-Host "当前失败在第2步 - 获取设备描述符"
Write-Host "这表明:"
Write-Host "✓ USB物理连接正常"
Write-Host "✓ 设备被检测到"
Write-Host "❌ 设备描述符读取失败"

Write-Host ""
Write-Host "可能的原因:"
Write-Host "1. 设备描述符内容错误"
Write-Host "2. 设备响应超时"
Write-Host "3. USB通信协议错误"
Write-Host "4. 电源供应不足"
Write-Host "5. 硬件故障"

Write-Host ""
Write-Host "=========================================="
Write-Host "下一步诊断重点:" -ForegroundColor Red
Write-Host "=========================================="

Write-Host ""
Write-Host "请提供以下关键信息:"
Write-Host ""
Write-Host "1. UART调试输出 (最重要!)"
Write-Host "   完整的从启动到USB插入的输出"
Write-Host ""
Write-Host "2. 设备管理器详细信息"
Write-Host "   设备属性中的硬件ID和错误详情"
Write-Host ""
Write-Host "3. 硬件配置确认"
Write-Host "   跳线设置和连接方式的照片"

Write-Host ""
Write-Host "根据这些信息，我们将:"
Write-Host "- 分析具体的失败点"
Write-Host "- 提供针对性的修复方案"  
Write-Host "- 如需要，使用极简USB描述符"

Write-Host ""
Write-Host "=========================================="
