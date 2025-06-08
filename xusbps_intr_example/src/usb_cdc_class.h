#ifndef XUSBPS_CLASS_CDC_H
#define XUSBPS_CLASS_CDC_H

#ifdef __cplusplus
extern "C"
{
#endif

/***************************** 包含文件 *********************************/
#include "xusbps.h" // 包含 XUsbPs 结构体定义, XUsbPs_SetupData, xusbps_hw.h, xil_types.h 等

/************************** 常量定义 *****************************/

// CDC 类特定请求代码 (bmRequestType: Class, Recipient: Interface)
#define XUSBPS_CDC_SEND_ENCAPSULATED_COMMAND 0x00
#define XUSBPS_CDC_GET_ENCAPSULATED_RESPONSE 0x01
#define XUSBPS_CDC_SET_COMM_FEATURE 0x02
#define XUSBPS_CDC_GET_COMM_FEATURE 0x03
#define XUSBPS_CDC_CLEAR_COMM_FEATURE 0x04
//                                        0x05-0x1F 保留
#define XUSBPS_CDC_SET_LINE_CODING 0x20
#define XUSBPS_CDC_GET_LINE_CODING 0x21
#define XUSBPS_CDC_SET_CONTROL_LINE_STATE 0x22
#define XUSBPS_CDC_SEND_BREAK 0x23

// CDC 通知代码 (bmRequestType: Class, Recipient: Interface, Direction: IN)
#define XUSBPS_CDC_NTF_NETWORK_CONNECTION 0x00
#define XUSBPS_CDC_NTF_RESPONSE_AVAILABLE 0x01
//                                        0x02-0x1F 保留
#define XUSBPS_CDC_NTF_SERIAL_STATE 0x20
//                                        0x21-0xFF 保留

/**************************** 类型定义 *******************************/

// USB CDC Line Coding 结构
// 用于 SET_LINE_CODING 和 GET_LINE_CODING 请求
// 字节顺序: Little Endian
#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
    typedef struct
    {
        u32 dwDTERate;  // 波特率 (例如 9600, 115200)
        u8 bCharFormat; // 停止位: 0 - 1 Stop bit, 1 - 1.5 Stop bits, 2 - 2 Stop bits
        u8 bParityType; // 校验位: 0 - None, 1 - Odd, 2 - Even, 3 - Mark, 4 - Space
        u8 bDataBits;   // 数据位: 5, 6, 7, 8 (或 16，但不常见)
#ifdef __ICCARM__
    } XUsbPs_CdcLineCoding;
#pragma pack(pop)
#else
} __attribute__((__packed__)) XUsbPs_CdcLineCoding;
#endif

// 用于 SET_CONTROL_LINE_STATE 请求的控制信号位掩码 (wValue 的低位)
#define XUSBPS_CDC_CTRL_STATE_DTR (1 << 0) // DTR (Data Terminal Ready)
#define XUSBPS_CDC_CTRL_STATE_RTS (1 << 1) // RTS (Request To Send)

// 用于 SERIAL_STATE 通知的状态位掩码 (2字节数据)
// 更多位定义见 USB CDC PSTN Subclass Spec (usbcdc11.pdf, Section 6.3.5)
#define XUSBPS_CDC_SERIAL_STATE_DCD (1 << 0)   // bRxCarrier (Data Carrier Detect)
#define XUSBPS_CDC_SERIAL_STATE_DSR (1 << 1)   // bTxCarrier (Data Set Ready)
#define XUSBPS_CDC_SERIAL_STATE_BREAK (1 << 2) // bBreak
#define XUSBPS_CDC_SERIAL_STATE_RI (1 << 3)    // bRingSignal
#define XUSBPS_CDC_SERIAL_STATE_FE (1 << 4)    // bFraming (Framing Error)
#define XUSBPS_CDC_SERIAL_STATE_PE (1 << 5)    // bParity (Parity Error)
#define XUSBPS_CDC_SERIAL_STATE_OE (1 << 6)    // bOverRun (Overrun Error)

    /************************** 函数原型 ******************************/

    // 初始化 CDC 类处理模块
    void XUsbPs_Cdc_Init(XUsbPs *InstancePtr);

    // 处理 CDC 类特定请求 (来自 EP0)
    s32 XUsbPs_Cdc_ClassReq(XUsbPs *InstancePtr, XUsbPs_SetupData *SetupData);

    // 获取当前的 Line Coding 设置
    // 此函数通常由 XUsbPs_Cdc_ClassReq 在处理 GET_LINE_CODING 时调用
    // 并将数据发送回主机。
    void XUsbPs_Cdc_GetLineCodingHandler(XUsbPs *InstancePtr);

    // 设置新的 Line Coding
    // 此函数通常由 XUsbPs_Cdc_ClassReq 在处理 SET_LINE_CODING 后调用
    // SetupData->Length 应该等于 sizeof(XUsbPs_CdcLineCoding)
    // 数据在 SetupData 之后的 EP0 缓冲区中
    // 返回值: XST_SUCCESS 或 XST_FAILURE
    s32 XUsbPs_Cdc_SetLineCodingHandler(XUsbPs *InstancePtr, u8 *Data, u32 Length);

    // 设置控制线状态 (DTR, RTS)
    // 此函数通常由 XUsbPs_Cdc_ClassReq 在处理 SET_CONTROL_LINE_STATE 时调用
    // ControlSignalBitmap 来自 SetupData->wValue
    void XUsbPs_Cdc_SetControlLineStateHandler(XUsbPs *InstancePtr, u16 ControlSignalBitmap);

    // 通过 CDC 数据 IN 端点发送数据
    s32 XUsbPs_Cdc_SendData(XUsbPs *InstancePtr, u8 *BufferPtr, u32 BufferLen);

    // CDC 数据 OUT 端点接收到数据后的回调函数
    // 由端点事件处理程序调用
    void XUsbPs_Cdc_ReceiveDataHandler(XUsbPs *InstancePtr, u8 *BufferPtr, u32 BufferLen);

    // 发送 SERIAL_STATE 通知到主机 (如果需要)
    s32 XUsbPs_Cdc_SendSerialState(XUsbPs *InstancePtr, u16 SerialStateBitmap);

#ifdef __cplusplus
}
#endif

#endif /* XUSBPS_CLASS_CDC_H */
