/******************************************************************************
 * 版权所有 (C) 2023 Advanced Micro Devices, Inc. 保留所有权利。
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

/*****************************************************************************/
/**
 * @file xusbps_class_cdc.c
 *
 * 此文件包含 USB CDC (通信设备类) - ACM (抽象控制模型) 的类特定请求处理实现。
 *
 *<pre>
 * 修改历史:
 *
 * 版本  谁   日期     更改
 * ----- ---- -------- ---------------------------------------------------------
 * 1.00   renversement 2023/06/08 初始版本，用于虚拟串口
 *</pre>
 ******************************************************************************/

/***************************** 包含文件 *********************************/
#include "usb_cdc_class.h"
#include "usb_ch9_handler.h" // 需要访问 XUsbPs_EpStall, XUsbPs_EpBufferSend 等
#include "usb_descriptors.h" // 需要访问 CDC 端点定义
#include <string.h>          // 用于 memcpy, memset
#include <xil_printf.h>      // 用于调试打印

/************************** 常量定义 *****************************/
// 默认 Line Coding 设置 (示例: 115200 8N1)
#define DEFAULT_LINE_CODING_RATE 115200
#define DEFAULT_LINE_CODING_CHARFORMAT 0x00 // 1 停止位
#define DEFAULT_LINE_CODING_PARITY 0x00     // 无校验
#define DEFAULT_LINE_CODING_DATABITS 0x08   // 8 数据位

/***************** 宏 (内联函数) 定义 *********************/

/**************************** 类型定义 *******************************/

// CDC 类实例数据结构
// 可以扩展以包含更多特定于 CDC 的状态信息
typedef struct
{
    XUsbPs_CdcLineCoding LineCoding; // 当前线路编码设置
    u16 ControlLineState;            // 当前控制线状态 (DTR, RTS)
    u8 IsHostConnected;              // 主机是否已连接并配置了 CDC (例如，DTR 是否已设置)
    // 可以添加用于数据缓冲的变量等
} XUsbPs_CdcInstance;

/************************** 静态函数原型 ******************************/

/************************** 变量定义 *****************************/

// CDC 类实例
// 注意: 如果有多个 USB 控制器实例或多个 CDC 接口，则需要更复杂的管理
static XUsbPs_CdcInstance CdcData;

// 用于 GET_LINE_CODING 的临时缓冲区
// 主机将通过控制读取阶段获取此数据
static XUsbPs_CdcLineCoding Ep0LineCodingBuffer;

/*****************************************************************************/
/**
 * @brief 初始化 CDC 类处理模块。
 *
 * @param InstancePtr 是指向 XUsbPs 实例的指针 (未使用，但保持 API 一致性)。
 *
 * @return 无。
 ******************************************************************************/
void XUsbPs_Cdc_Init(XUsbPs *InstancePtr)
{
    (void)InstancePtr; // 避免编译器警告

    // 初始化 Line Coding 为默认值
    CdcData.LineCoding.dwDTERate = DEFAULT_LINE_CODING_RATE;
    CdcData.LineCoding.bCharFormat = DEFAULT_LINE_CODING_CHARFORMAT;
    CdcData.LineCoding.bParityType = DEFAULT_LINE_CODING_PARITY;
    CdcData.LineCoding.bDataBits = DEFAULT_LINE_CODING_DATABITS;

    CdcData.ControlLineState = 0;
    CdcData.IsHostConnected = 0;

    xil_printf("CDC Class Initialized. Default Line Coding: %d %d%c%d\r\n",
               (int)CdcData.LineCoding.dwDTERate,
               CdcData.LineCoding.bDataBits,
               (CdcData.LineCoding.bParityType == 0) ? 'N' : (CdcData.LineCoding.bParityType == 1) ? 'O'
                                                         : (CdcData.LineCoding.bParityType == 2)   ? 'E'
                                                                                                   : '?',
               (CdcData.LineCoding.bCharFormat == 0) ? 1 : (CdcData.LineCoding.bCharFormat == 2) ? 2
                                                                                                 : 15); // 1.5 停止位用 15 表示
}

/*****************************************************************************/
/**
 * @brief 处理 CDC 类特定请求。
 *
 * @param InstancePtr 是指向 XUsbPs 实例的指针。
 * @param SetupData 是指向设置数据包的指针。
 *
 * @return XST_SUCCESS 如果请求被处理，否则返回 XST_FAILURE。
 ******************************************************************************/
s32 XUsbPs_Cdc_ClassReq(XUsbPs *InstancePtr, XUsbPs_SetupData *SetupData)
{
    s32 Status = XST_SUCCESS;
    u32 ReplyLen;
    
    printf("CDC Class Request: Type=0x%02x, Req=0x%02x, Val=0x%04x, Idx=0x%04x, Len=%d\r\n",
           SetupData->bmRequestType, SetupData->bRequest, SetupData->wValue, 
           SetupData->wIndex, SetupData->wLength);

    // 确保请求是针对 CDC 控制接口 (通常是接口 0)
    // SetupData->wIndex 的低字节应该是接口号
    if ((SetupData->wIndex & 0xFF) != 0x00)
    { // 假设 CDC 控制接口是 0
        // 不是针对我们的控制接口，可能针对其他接口或无效请求
        printf("CDC ClassReq: Request for wrong interface %x\r\n", SetupData->wIndex);
        // return XST_FAILURE; // 或者让上层处理 STALL
    }

    switch (SetupData->bRequest)
    {    case XUSBPS_CDC_SET_LINE_CODING:
        // 主机希望设置线路编码
        // 数据将在控制 OUT 阶段发送
        printf("CDC Req: SET_LINE_CODING, len %d\r\n", SetupData->wLength);
        if (SetupData->wLength != sizeof(XUsbPs_CdcLineCoding))
        {
            // 长度错误
            printf("ERROR: SET_LINE_CODING invalid length: %d (expected %d)\r\n", 
                   SetupData->wLength, sizeof(XUsbPs_CdcLineCoding));
            // XUsbPs_EpStall(InstancePtr, 0, XUSBPS_EP_DIRECTION_OUT);
            Status = XST_FAILURE;
            break;
        }
        // 准备接收数据到 EP0 缓冲区，然后在 XUsbPs_Cdc_SetLineCodingHandler 中处理
        // 驱动程序通常会自动处理数据阶段的接收
        // 我们将在数据接收完成后在 SetLineCodingHandler 中被调用
        // 这里我们只需要确认请求，并准备好在数据阶段后处理它
        // XUsbPs_EpBufferReceive(InstancePtr, 0, InstancePtr->Ep0DataBuffer, SetupData->wLength);
        // 状态阶段由驱动处理
        break;    case XUSBPS_CDC_GET_LINE_CODING:
        // 主机希望获取线路编码
        printf("CDC Req: GET_LINE_CODING\r\n");
        ReplyLen = sizeof(XUsbPs_CdcLineCoding);
        if (SetupData->wLength < ReplyLen)
        {
            // 主机请求的长度不足
            printf("ERROR: GET_LINE_CODING insufficient length: %d (needed %d)\r\n",
                   SetupData->wLength, ReplyLen);
            // XUsbPs_EpStall(InstancePtr, 0, XUSBPS_EP_DIRECTION_IN);
            Status = XST_FAILURE;
            break;
        }
        // 将当前的 LineCoding 复制到临时缓冲区，然后发送
        printf("Sending LINE_CODING: rate=%d, format=%d, parity=%d, bits=%d\r\n",
               CdcData.LineCoding.dwDTERate, CdcData.LineCoding.bCharFormat,
               CdcData.LineCoding.bParityType, CdcData.LineCoding.bDataBits);
        memcpy(&Ep0LineCodingBuffer, &CdcData.LineCoding, ReplyLen);
        XUsbPs_EpBufferSend(InstancePtr, 0, (u8 *)&Ep0LineCodingBuffer, ReplyLen);
        break;    case XUSBPS_CDC_SET_CONTROL_LINE_STATE:
        // 主机设置控制线状态 (DTR, RTS)
        printf("CDC Req: SET_CONTROL_LINE_STATE, value 0x%04X\r\n", SetupData->wValue);
        XUsbPs_Cdc_SetControlLineStateHandler(InstancePtr, SetupData->wValue);
        // 此请求通常没有数据阶段，只有状态阶段
        XUsbPs_EpBufferSend(InstancePtr, 0, NULL, 0); // 完成状态阶段
        break;    case XUSBPS_CDC_SEND_ENCAPSULATED_COMMAND: // 0x00
    case XUSBPS_CDC_GET_ENCAPSULATED_RESPONSE: // 0x01
    case XUSBPS_CDC_SET_COMM_FEATURE:          // 0x02
    case XUSBPS_CDC_GET_COMM_FEATURE:          // 0x03
    case XUSBPS_CDC_CLEAR_COMM_FEATURE:        // 0x04
    case XUSBPS_CDC_SEND_BREAK:                // 0x23
        // 这些是可选的 CDC 请求，当前示例中未实现
        printf("CDC Req: Unhandled/Optional request 0x%02X\r\n", SetupData->bRequest);
        // 对于未处理的请求，通常应该 STALL 端点0
        // XUsbPs_EpStall(InstancePtr, 0, XUSBPS_EP_DIRECTION_OUT); // 或 IN，取决于请求方向
        Status = XST_FAILURE; // 指示上层处理 STALL
        break;

    default:
        // 未知或不支持的类请求
        // xil_printf("CDC Req: Unknown class request 0x%02X\r\n", SetupData->bRequest);
        Status = XST_FAILURE; // 指示上层处理 STALL
        break;
    }
    return Status;
}

/*****************************************************************************/
/**
 * @brief 处理 GET_LINE_CODING 请求 (实际由 XUsbPs_Cdc_ClassReq 调用)。
 *        此函数主要用于演示，实际数据发送在 ClassReq 中完成。
 *
 * @param InstancePtr 是指向 XUsbPs 实例的指针。
 ******************************************************************************/
void XUsbPs_Cdc_GetLineCodingHandler(XUsbPs *InstancePtr)
{
    // 实际逻辑在 XUsbPs_Cdc_ClassReq 中处理 GET_LINE_CODING
    // 这里只是一个占位符或可以用于更新内部状态（如果需要）
    (void)InstancePtr;
    // xil_printf("GET_LINE_CODING handled (data sent by ClassReq)\r\n");
}

/*****************************************************************************/
/**
 * @brief 处理 SET_LINE_CODING 请求的数据阶段后。
 *
 * @param InstancePtr 是指向 XUsbPs 实例的指针。
 * @param Data 是指向包含新 Line Coding 数据的缓冲区的指针。
 * @param Length 是数据的长度 (应为 sizeof(XUsbPs_CdcLineCoding))。
 *
 * @return XST_SUCCESS 如果成功，XST_FAILURE 如果参数无效。
 ******************************************************************************/
s32 XUsbPs_Cdc_SetLineCodingHandler(XUsbPs *InstancePtr, u8 *Data, u32 Length)
{
    (void)InstancePtr; // 避免编译器警告

    if (Data == NULL || Length != sizeof(XUsbPs_CdcLineCoding))
    {
        // xil_printf("SET_LINE_CODING Handler: Invalid data or length\r\n");
        return XST_FAILURE;
    }

    memcpy(&CdcData.LineCoding, Data, sizeof(XUsbPs_CdcLineCoding));

    // xil_printf("SET_LINE_CODING: Rate=%d, Format=%d, Parity=%d, Bits=%d\r\n",
    //            (int)CdcData.LineCoding.dwDTERate,
    //            CdcData.LineCoding.bCharFormat,
    //            CdcData.LineCoding.bParityType,
    //            CdcData.LineCoding.bDataBits);

    // TODO: 根据新的 Line Coding 设置实际配置 UART (如果硬件 UART 与此 CDC 关联)
    // 例如: XUartPs_SetBaudRate, XUartPs_SetDataFormat 等

    return XST_SUCCESS;
}

/*****************************************************************************/
/**
 * @brief 处理 SET_CONTROL_LINE_STATE 请求。
 *
 * @param InstancePtr 是指向 XUsbPs 实例的指针 (未使用)。
 * @param ControlSignalBitmap 是来自 SetupData->wValue 的控制信号。
 ******************************************************************************/
void XUsbPs_Cdc_SetControlLineStateHandler(XUsbPs *InstancePtr, u16 ControlSignalBitmap)
{
    (void)InstancePtr;
    CdcData.ControlLineState = ControlSignalBitmap;

    printf("SET_CONTROL_LINE_STATE: DTR=%d, RTS=%d\r\n",
               (CdcData.ControlLineState & XUSBPS_CDC_CTRL_STATE_DTR) ? 1 : 0,
               (CdcData.ControlLineState & XUSBPS_CDC_CTRL_STATE_RTS) ? 1 : 0);

    // 根据 DTR 状态更新连接状态
    if (CdcData.ControlLineState & XUSBPS_CDC_CTRL_STATE_DTR)
    {
        CdcData.IsHostConnected = 1;
        printf("CDC Host Connected (DTR set)\r\n");
        // TODO: 执行连接建立时的操作，例如准备接收数据
    }
    else
    {
        CdcData.IsHostConnected = 0;
        printf("CDC Host Disconnected (DTR cleared)\r\n");
        // TODO: 执行连接断开时的操作
    }

    // RTS 通常由设备控制，表示设备是否准备好接收数据。
    // 主机设置 RTS 通常是为了流控制，但在这个简单示例中我们可能不完全实现它。
}

/*****************************************************************************/
/**
 * @brief 通过 CDC 数据 IN 端点发送数据。
 *
 * @param InstancePtr 是指向 XUsbPs 实例的指针。
 * @param BufferPtr 是指向要发送的数据缓冲区的指针。
 * @param BufferLen 是要发送的数据长度。
 *
 * @return XST_SUCCESS 如果数据成功排队等待发送，否则返回错误代码。
 ******************************************************************************/
s32 XUsbPs_Cdc_SendData(XUsbPs *InstancePtr, u8 *BufferPtr, u32 BufferLen)
{
    if (!CdcData.IsHostConnected)
    {
        // xil_printf("CDC SendData: Host not connected (DTR not set)\r\n");
        return XST_FAILURE; // 或者根据情况返回其他错误
    }

    if (BufferLen == 0)
    {
        return XST_SUCCESS; // 无数据发送
    }

    // xil_printf("CDC SendData: Sending %d bytes on EP%d IN\r\n", (int)BufferLen, CDC_DATA_IN_EP);

    // 使用 XUsbPs_EpBufferSend 将数据发送到 Bulk IN 端点 (EP1 IN)
    // 注意: CDC_DATA_IN_EP 应该与 xusbps_ch9_storage.c 中定义的端点号一致
    return XUsbPs_EpBufferSend(InstancePtr, CDC_DATA_IN_EP, BufferPtr, BufferLen);
}

/*****************************************************************************/
/**
 * @brief CDC 数据 OUT 端点接收到数据后的回调函数。
 *
 * @param InstancePtr 是指向 XUsbPs 实例的指针。
 * @param BufferPtr 是指向接收到的数据缓冲区的指针。
 * @param BufferLen 是接收到的数据长度。
 ******************************************************************************/
void XUsbPs_Cdc_ReceiveDataHandler(XUsbPs *InstancePtr, u8 *BufferPtr, u32 BufferLen)
{
    /* 抑制未使用参数警告 */
    (void)InstancePtr;
    (void)BufferPtr;
    (void)BufferLen;
    
    // xil_printf("CDC ReceiveData: Received %d bytes on EP%d OUT\r\n", (int)BufferLen, CDC_DATA_OUT_EP);

    // TODO: 处理接收到的数据 (BufferPtr, BufferLen)
    // 例如，可以将数据回显，或传递给应用程序的其他部分

    // 示例: 回显数据 (如果需要)
    // if (BufferLen > 0) {
    //     XUsbPs_Cdc_SendData(InstancePtr, BufferPtr, BufferLen);
    // }

    // 重要: 接收完成后，需要重新 Prime (准备) Bulk OUT 端点以接收更多数据
    // XUsbPs_EpBufferReceive(InstancePtr, CDC_DATA_OUT_EP, DmaBuffer, MAX_TRANSFER_SIZE);
    // 实际的 BufferPtr 和其大小应由上层管理
    // 这里只是一个回调，表示数据已在 BufferPtr 中可用
    // 上层应用在处理完数据后，应再次调用 XUsbPs_EpBufferReceive 来准备下一次接收
}

/*****************************************************************************/
/**
 * @brief 发送 SERIAL_STATE 通知到主机 (如果需要)。
 *
 * @param InstancePtr 是指向 XUsbPs 实例的指针。
 * @param SerialStateBitmap 是要发送的串行状态位图。
 *
 * @return XST_SUCCESS 如果通知成功排队等待发送，否则返回错误代码。
 ******************************************************************************/
s32 XUsbPs_Cdc_SendSerialState(XUsbPs *InstancePtr, u16 SerialStateBitmap)
{
    // SERIAL_STATE 通知是一个包含2字节位掩码的输入报告
    // 通过 CDC 控制接口的中断 IN 端点 (EP2 IN) 发送
    // 通知格式: bmRequestType=0xA1, bNotificationCode=SERIAL_STATE (0x20),
    // wValue=0, wIndex=Interface (0), wLength=2, Data=SerialStateBitmap

    // 实际的 USB 通知结构
#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
    struct
    {
        u8 bmRequestType;     // 0xA1 (Class, Interface, Device-to-Host)
        u8 bNotificationCode; // 0x20 (SERIAL_STATE)
        u16 wValue;           // 0x0000
        u16 wIndex;           // Interface number (e.g., 0x0000 for Control Interface)
        u16 wLength;          // 0x0002 (size of data)
        u16 Data;             // Serial state bitmap
#ifdef __ICCARM__
    } __attribute__((__packed__)) NotificationPacket;
#pragma pack(pop)
#else
    } __attribute__((__packed__)) NotificationPacket;
#endif

    if (!CdcData.IsHostConnected)
    {
        // xil_printf("CDC SendSerialState: Host not connected\r\n");
        return XST_FAILURE;
    }

    NotificationPacket.bmRequestType = 0xA1;
    NotificationPacket.bNotificationCode = XUSBPS_CDC_NTF_SERIAL_STATE;
    NotificationPacket.wValue = 0;
    NotificationPacket.wIndex = 0;                                // CDC 控制接口号
    NotificationPacket.wLength = sizeof(NotificationPacket.Data); // 应该是 2
    NotificationPacket.Data = SerialStateBitmap;

    // xil_printf("CDC SendSerialState: Sending 0x%04X on EP%d IN (Notification)\r\n",
    //            SerialStateBitmap, CDC_NOTIFICATION_EP);

    // 发送通知
    return XUsbPs_EpBufferSend(InstancePtr, CDC_NOTIFICATION_EP,
                               (u8 *)&NotificationPacket, sizeof(NotificationPacket));
}
