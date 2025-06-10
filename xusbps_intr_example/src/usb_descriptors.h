/******************************************************************************
 * 版权所有 (C) 2010 - 2022 Xilinx, Inc. 保留所有权利。
 * 版权所有 (C) 2023 Advanced Micro Devices, Inc. 保留所有权利。
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file xusbps_ch9_storage.h
 *
 * 此文件包含第 9 章代码中使用的定义。
 *
 *
 * <pre>
 * 修改历史:
 *
 * 版本  谁   日期     更改
 * ----- ---- -------- -------------------------------------------------------
 * 1.00a wgr  10/10/10 首次发布
 * 2.5	 pm   02/20/20 添加了 SetConfigurationApp 和 SetInterfaceHandler API
 * </pre>
 *
 ******************************************************************************/

#ifndef XUSBPS_CH9_STORAGE_H
#define XUSBPS_CH9_STORAGE_H

#ifdef __cplusplus
extern "C"
{
#endif

    /***************************** 包含文件 *********************************/

#include "xusbps_hw.h"
#include "xil_types.h"
#include "xstatus.h"

/************************** 常量定义 *****************************/

/* USB 类代码 */
#define XUSBPS_CLASS_CDC 0x02
#define XUSBPS_CLASS_CDC_DATA 0x0A

/* CDC 子类代码 */
#define XUSBPS_CDC_SUBCLASS_ACM 0x02

/* CDC 协议代码 */
#define XUSBPS_CDC_PROTOCOL_V25TER 0x01 // AT 命令, V.25ter

/* CDC 描述符类型 */
#define XUSBPS_TYPE_CS_INTERFACE 0x24
#define XUSBPS_TYPE_CS_ENDPOINT 0x25

/* CDC 功能描述符子类型 */
#define XUSBPS_CDC_FND_HEADER 0x00
#define XUSBPS_CDC_FND_CALL_MGMT 0x01
#define XUSBPS_CDC_FND_ACM 0x02
#define XUSBPS_CDC_FND_UNION 0x06

/* CDC 端点定义 */
#define CDC_DATA_OUT_EP 1     // 数据OUT端点 (EP1 OUT)
#define CDC_DATA_IN_EP 1      // 数据IN端点 (EP1 IN)
#define CDC_NOTIFICATION_EP 2 // 通知端点 (EP2 IN)

/* USB 端点控制寄存器相关宏定义 */
#define XUSBPS_EPCR_OFFSET 0x4                  // 端点控制寄存器偏移量
#define XUSBPS_EPCR_RXT_MASK 0x000000C0         // RX传输类型掩码
#define XUSBPS_EPCR_TXT_MASK 0x000C0000         // TX传输类型掩码
#define XUSBPS_EPCR_RXT_DISABLE_MASK 0x00000000 // RX禁用
#define XUSBPS_EPCR_TXT_INT_MASK 0x00080000     // TX中断传输

/* 工具宏定义 */
#ifdef __LITTLE_ENDIAN__
    #define be2les(val) (val) // 小端字节序不需要转换
#else
    #define be2les(val) (((val) >> 8) | ((val) << 8)) // 大端字节序需要转换
#endif

// ARM处理器通常是小端字节序，但为了确保正确性，我们明确定义
#ifndef __LITTLE_ENDIAN__
    #define __LITTLE_ENDIAN__
#endif
#define le16(val) (val) // 明确的小端字节序宏

    /**************************** 类型定义 *******************************/

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
    typedef struct
    {
        u8 bLength;
        u8 bDescriptorType;
        u16 wTotalLength;
        u8 bNumInterfaces;
        u8 bConfigurationValue;
        u8 iConfiguration;
        u8 bmAttributes;
        u8 bMaxPower;
#ifdef __ICCARM__
    } USB_STD_CFG_DESC;
#pragma pack(pop)
#else
} __attribute__((__packed__)) USB_STD_CFG_DESC;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
    // CDC 接口关联描述符 (IAD)
    typedef struct
    {
        u8 bLength;           // 描述符大小 (8 字节)
        u8 bDescriptorType;   // IAD 描述符类型 (0x0B)
        u8 bFirstInterface;   // 第一个接口号
        u8 bInterfaceCount;   // 接口数量 (通常为 2: 控制 + 数据)
        u8 bFunctionClass;    // 功能类代码 (0x02 - CDC)
        u8 bFunctionSubClass; // 功能子类代码 (0x02 - ACM)
        u8 bFunctionProtocol; // 功能协议代码 (例如 0x01 - V.25ter)
        u8 iFunction;         // 描述此功能的字符串描述符索引
#ifdef __ICCARM__
    } USB_STD_IAD_DESC;
#pragma pack(pop)
#else
} __attribute__((__packed__)) USB_STD_IAD_DESC;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
    // CDC 接口描述符
    typedef struct
    {
        u8 bLength;            // 描述符大小
        u8 bDescriptorType;    // 接口描述符类型 (0x04)
        u8 bInterfaceNumber;   // 接口号
        u8 bAlternateSetting;  // 备用接口设置
        u8 bNumEndpoints;      // 此接口的端点数量
        u8 bInterfaceClass;    // 接口类代码 (0x02 - CDC)
        u8 bInterfaceSubClass; // 接口子类代码 (0x02 - ACM)
        u8 bInterfaceProtocol; // 接口协议代码 (例如 0x01 - V.25ter)
        u8 iInterface;         // 描述此接口的字符串描述符索引
#ifdef __ICCARM__
    } USB_STD_IF_DESC;
#pragma pack(pop)
#else
} __attribute__((__packed__)) USB_STD_IF_DESC;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
    // CDC 端点描述符
    typedef struct
    {
        u8 bLength;          // 描述符大小
        u8 bDescriptorType;  // 端点描述符类型 (0x05)
        u8 bEndpointAddress; // 端点地址 (包含方向位)
        u8 bmAttributes;     // 端点属性
        u16 wMaxPacketSize;  // 最大包大小
        u8 bInterval;        // 间隔时间
#ifdef __ICCARM__
    } USB_STD_EP_DESC;
#pragma pack(pop)
#else
} __attribute__((__packed__)) USB_STD_EP_DESC;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
    // CDC Header Functional Descriptor
    typedef struct
    {
        u8 bFunctionLength;    // 描述符大小
        u8 bDescriptorType;    // CS_INTERFACE (0x24)
        u8 bDescriptorSubtype; // HEADER (0x00)
        u16 bcdCDC;            // CDC 版本 (例如 0x0110 for 1.10)
#ifdef __ICCARM__
    } USB_CDC_HEADER_FND_DESC;
#pragma pack(pop)
#else
} __attribute__((__packed__)) USB_CDC_HEADER_FND_DESC;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
    // CDC Call Management Functional Descriptor
    typedef struct
    {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype; // CALL_MGMT (0x01)
        u8 bmCapabilities;     // 能力
        u8 bDataInterface;     // 数据类接口的接口号
#ifdef __ICCARM__
    } USB_CDC_CALL_MGMT_FND_DESC;
#pragma pack(pop)
#else
} __attribute__((__packed__)) USB_CDC_CALL_MGMT_FND_DESC;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
    // CDC Abstract Control Management Functional Descriptor
    typedef struct
    {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype; // ACM (0x02)
        u8 bmCapabilities;     // 能力
#ifdef __ICCARM__
    } USB_CDC_ACM_FND_DESC;
#pragma pack(pop)
#else
} __attribute__((__packed__)) USB_CDC_ACM_FND_DESC;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
    // CDC Union Functional Descriptor
    typedef struct
    {
        u8 bFunctionLength;
        u8 bDescriptorType;
        u8 bDescriptorSubtype; // UNION (0x06)
        u8 bMasterInterface;   // 控制接口的接口号 (bControlInterface)
        u8 bSlaveInterface0;   // 第一个从接口的接口号 (bSubordinateInterface)
#ifdef __ICCARM__
    } USB_CDC_UNION_FND_DESC;
#pragma pack(pop)
#else
} __attribute__((__packed__)) USB_CDC_UNION_FND_DESC;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
    // 字符串描述符
    typedef struct
    {
        u8 bLength;
        u8 bDescriptorType;
        u16 wString[1]; // 字符串内容 (UTF-16LE 编码)
#ifdef __ICCARM__
    } USB_STD_STRING_DESC;
#pragma pack(pop)
#else
} __attribute__((__packed__)) USB_STD_STRING_DESC;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
    // 更新配置描述符结构以包含 CDC 接口和端点
    typedef struct
    {
        USB_STD_CFG_DESC stdCfg;
        USB_STD_IAD_DESC iad;
        // CDC 控制接口
        USB_STD_IF_DESC ifCtrl;
        USB_CDC_HEADER_FND_DESC cdcHeader;
        USB_CDC_CALL_MGMT_FND_DESC cdcCallMgmt;
        USB_CDC_ACM_FND_DESC cdcAcm;
        USB_CDC_UNION_FND_DESC cdcUnion;
        USB_STD_EP_DESC epCtrl;
        // CDC 数据接口
        USB_STD_IF_DESC ifData;
        USB_STD_EP_DESC epDataOut;
        USB_STD_EP_DESC epDataIn;
#ifdef __ICCARM__
    } USB_CDC_CONFIG_DESC_FULL;
#pragma pack(pop)
#else
} __attribute__((__packed__)) USB_CDC_CONFIG_DESC_FULL;
#endif

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
    // 原来的 USB_CONFIG 结构，现在可能不再直接使用，或者需要调整
    // typedef struct {
    // 	USB_STD_CFG_DESC stdCfg;
    // 	USB_STD_IF_DESC ifCfg;
    // 	USB_STD_EP_DESC epCfg1;
    // 	USB_STD_EP_DESC epCfg2;
    // #ifdef __ICCARM__
    // } USB_CONFIG;
    // #pragma pack(pop)
    // #else
    // } __attribute__((__packed__))USB_CONFIG;
    // #endif

    /************************** 函数原型 ******************************/

    u32 XUsbPs_Ch9SetupDevDescReply(u8 *BufPtr, u32 BufLen);
    u32 XUsbPs_Ch9SetupCfgDescReply(u8 *BufPtr, u32 BufLen);
    u32 XUsbPs_Ch9SetupStrDescReply(u8 *BufPtr, u32 BufLen, u8 Index);
    void XUsbPs_SetConfiguration(XUsbPs *InstancePtr, int ConfigIdx);
    void XUsbPs_SetConfigurationApp(XUsbPs *InstancePtr,
                                    XUsbPs_SetupData *SetupData);
    void XUsbPs_SetInterfaceHandler(XUsbPs *InstancePtr,
                                    XUsbPs_SetupData *SetupData);

#ifdef __cplusplus
}
#endif

#endif /* XUSBPS_CH9_STORAGE_H */
