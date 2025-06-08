/******************************************************************************
* Copyright (C) 2010 - 2022 Xilinx, Inc.  All rights reserved.
* Copyright (C) 2023 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
*****************************************************

/*****************************************************************************/
/**
* @file xusbps_intr_example.c
*
* 此文件包含如何在设备模式下使用USB驱动程序和USB控制器的示例。
*
/*****************************************************************************/
/**
 * 此函数注册用于处理端点 1（CDC 数据端点）的回调。
 *
 * 它在中断上下文中被调用，因此应最大限度地减少执行的处理量。
 *
 * 此函数处理 CDC 数据端点的数据收发。
 *
 * @param CallBackRef 是注册函数时传入的引用。
 * @param EpNum 是发生事件的端点的编号。
 * @param EventType 是发生的事件的类型。
 *
 * @return 无。
 *
 * @note 无。
 *
 ******************************************************************************/

/***************************** 包含文件 *********************************/

#include "xparameters.h"	 /* XPAR 参数 */
#include "xusbps.h"			 /* USB控制器驱动 */
#include "usb_ch9_handler.h" /* 通用第9章处理代码 */
#include "usb_cdc_class.h"	 /* CDC 类处理代码 */
#include "xil_exception.h"
#include "xpseudo_asm.h"
#include "xreg_cortexa9.h"
#include "xil_cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sleep.h" /* 用于usleep函数 */

#ifndef SDT
#include "xscugic.h"
#else
#include "xinterrupt_wrap.h"
#endif

/************************** 常量定义 *****************************/
#define MEMORY_SIZE (64 * 1024)
#ifdef __ICCARM__
#pragma data_alignment = 32
u8 Buffer[MEMORY_SIZE];
#else
u8 Buffer[MEMORY_SIZE] ALIGNMENT_CACHELINE;
#endif

/**************************** 类型定义 *******************************/

/***************** 宏定义（内联函数）定义 *********************/
#ifdef SDT
#define USBPS_BASEADDR XPS_USB0_BASEADDR /* USBPS基地址 */
#endif

/************************** 函数原型 ******************************/
#ifndef SDT
static int UsbIntrExample(XScuGic *IntcInstancePtr, XUsbPs *UsbInstancePtr,
						  u16 UsbDeviceId, u16 UsbIntrId);
static void UsbDisableIntrSystem(XScuGic *IntcInstancePtr, u16 UsbIntrId);
static int UsbSetupIntrSystem(XScuGic *IntcInstancePtr,
							  XUsbPs *UsbInstancePtr, u16 UsbIntrId);
#else
static int UsbIntrExample(XUsbPs *UsbInstancePtr, UINTPTR BaseAddress);
#endif

static void UsbIntrHandler(void *CallBackRef, u32 Mask);
static void XUsbPs_Ep0EventHandler(void *CallBackRef, u8 EpNum,
								   u8 EventType, void *Data);
static void XUsbPs_Ep1EventHandler(void *CallBackRef, u8 EpNum,
								   u8 EventType, void *Data);
static void XUsbPs_Ep2EventHandler(void *CallBackRef, u8 EpNum,
								   u8 EventType, void *Data);

/************************** 变量定义 *****************************/

/* 支持设备驱动程序的实例是全局的，因此每次程序运行时都会初始化为零。
 */
static XScuGic IntcInstance; /* IRQ控制器实例 */
static XUsbPs UsbInstance;	 /* USB控制器实例 */

static volatile int NumIrqs = 0;
static volatile int NumReceivedFrames = 0;
static volatile int DeviceConnected = 0;
static volatile int DeviceConfigured = 0;

/*****************************************************************************/
/**
 *
 * 调用USB中断示例的主函数。
 *
 * @param	无
 *
 * @return
 * 		- XST_SUCCESS 如果成功
 * 		- XST_FAILURE 出错时
 *
 ******************************************************************************/

int main(void)
{
	int Status;

	/* 运行USB中断示例。*/
#ifndef SDT
	Status = UsbIntrExample(&IntcInstance, &UsbInstance,
							XPAR_XUSBPS_0_DEVICE_ID, XPAR_XUSBPS_0_INTR);
#else
	Status = UsbIntrExample(&UsbInstance, USBPS_BASEADDR);
#endif
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
 *
 * 此函数在USB设备和驱动程序上执行最小的设备模式设置，作为设计示例。
 * 此函数的目的是说明如何设置USB闪存盘仿真系统。
 *
 *
 * @param	IntcInstancePtr 是指向INTC驱动程序实例的指针。
 * @param	UsbInstancePtr 是指向USB驱动程序实例的指针。
 * @param	UsbDeviceId 是USB控制器的设备ID，是xparameters.h中的
 * 		XPAR_<USB_instance>_DEVICE_ID值。
 * @param	UsbIntrId 是中断ID，通常是xparameters.h中的
 * 		XPAR_<INTC_instance>_<USB_instance>_IP2INTC_IRPT_INTR值。
 *
 * @return
 * 		- XST_SUCCESS 如果成功
 * 		- XST_FAILURE 出错时
 *
 ******************************************************************************/
#ifndef SDT
static int UsbIntrExample(XScuGic *IntcInstancePtr, XUsbPs *UsbInstancePtr,
						  u16 UsbDeviceId, u16 UsbIntrId)
#else
static int UsbIntrExample(XUsbPs *UsbInstancePtr, UINTPTR BaseAddress)
#endif
{
	int Status;
	u8 *MemPtr = NULL;
	int ReturnStatus = XST_FAILURE;

	/* 在这个示例中我们配置3个端点用于 CDC ACM：
	 *   端点0（默认控制端点）
	 *   端点1（CDC 数据端点 - Bulk IN/OUT）
	 *   端点2（CDC 通知端点 - Interrupt IN）
	 */
	const u8 NumEndpoints = 3;

	XUsbPs_Config *UsbConfigPtr;
	XUsbPs_DeviceConfig DeviceConfig;

	/* 初始化USB驱动程序，使其准备就绪，
	 * 指定在xparameters.h中生成的控制器ID
	 */
#ifndef SDT
	UsbConfigPtr = XUsbPs_LookupConfig(UsbDeviceId);
#else
	UsbConfigPtr = XUsbPs_LookupConfig(BaseAddress);
#endif
	if (NULL == UsbConfigPtr)
	{
		goto out;
	}

	/* 我们将物理基地址作为第三个参数传递，
	 * 因为在我们的示例中物理和虚拟基地址是相同的。
	 * 对于支持虚拟内存的系统，第三个参数需要是虚拟基地址。
	 */
	Status = XUsbPs_CfgInitialize(UsbInstancePtr,
								  UsbConfigPtr,
								  UsbConfigPtr->BaseAddress);
	if (XST_SUCCESS != Status)
	{
		goto out;
	}

	/* 设置中断子系统。
	 */
#ifndef SDT
	Status = UsbSetupIntrSystem(IntcInstancePtr,
								UsbInstancePtr,
								UsbIntrId);
#else
	Status = XSetupInterruptSystem(UsbInstancePtr, &XUsbPs_IntrHandler,
								   UsbConfigPtr->IntrId,
								   UsbConfigPtr->IntrParent,
								   XINTERRUPT_DEFAULT_PRIORITY);
#endif
	if (XST_SUCCESS != Status)
	{
		goto out;
	}

	/* 控制器设备端的配置分多个阶段进行。
	 *
	 * 1) 用户使用XUsbPs_DeviceConfig数据结构配置所需的端点配置。
	 * 这包括端点数量、每个端点的传输描述符数量（每个端点可以有不同数量的传输描述符）
	 * 以及OUT（接收）端点的缓冲区大小。每个端点可以有不同的缓冲区大小。
	 *
	 * 2) 使用XUsbPs_DeviceMemRequired()调用从驱动程序请求所需的DMA内存大小。
	 *
	 * 3) 分配DMA内存并在XUsbPs_DeviceConfig数据结构中设置DMAMemVirt和DMAMemPhys成员。
	 *
	 * 4) 通过调用XUsbPs_ConfigureDevice()函数配置控制器的设备端。
	 */

	/*
	 * 配置端点 0（控制端点）
	 * 端点0是控制端点，用于处理设备配置和 CDC 类特定请求
	 */
	DeviceConfig.EpCfg[0].Out.Type = XUSBPS_EP_TYPE_CONTROL;
	DeviceConfig.EpCfg[0].Out.NumBufs = 2;
	DeviceConfig.EpCfg[0].Out.BufSize = 64;
	DeviceConfig.EpCfg[0].Out.MaxPacketSize = 64;
	DeviceConfig.EpCfg[0].In.Type = XUSBPS_EP_TYPE_CONTROL;
	DeviceConfig.EpCfg[0].In.NumBufs = 2;
	DeviceConfig.EpCfg[0].In.MaxPacketSize = 64;

	/*
	 * 配置端点 1（CDC 数据端点）
	 * 此端点用于 CDC 数据传输（虚拟串口数据）
	 * 使用64字节包大小确保兼容性，硬件将自动协商到最高支持速度
	 */
	DeviceConfig.EpCfg[1].Out.Type = XUSBPS_EP_TYPE_BULK;
	DeviceConfig.EpCfg[1].Out.NumBufs = 16;
	DeviceConfig.EpCfg[1].Out.BufSize = 512;  // 缓冲区保持大，支持高速传输
	DeviceConfig.EpCfg[1].Out.MaxPacketSize = 64;  // 描述符包大小兼容
	DeviceConfig.EpCfg[1].In.Type = XUSBPS_EP_TYPE_BULK;
	DeviceConfig.EpCfg[1].In.NumBufs = 16;
	DeviceConfig.EpCfg[1].In.MaxPacketSize = 64;

	/*
	 * 配置端点 2（CDC 通知端点）
	 * 此端点用于 CDC 通知（如串行状态变化）
	 */
	DeviceConfig.EpCfg[2].Out.Type = XUSBPS_EP_TYPE_INTERRUPT;
	DeviceConfig.EpCfg[2].Out.NumBufs = 2;
	DeviceConfig.EpCfg[2].Out.BufSize = 16;
	DeviceConfig.EpCfg[2].Out.MaxPacketSize = 16;
	DeviceConfig.EpCfg[2].In.Type = XUSBPS_EP_TYPE_INTERRUPT;
	DeviceConfig.EpCfg[2].In.NumBufs = 2;
	DeviceConfig.EpCfg[2].In.MaxPacketSize = 16;

	DeviceConfig.NumEndpoints = NumEndpoints;

	MemPtr = (u8 *)&Buffer[0];
	memset(MemPtr, 0, MEMORY_SIZE);
	Xil_DCacheFlushRange((unsigned int)MemPtr, MEMORY_SIZE);

	/* 完成DeviceConfig结构的配置并配置控制器的设备端。
	 */
	DeviceConfig.DMAMemPhys = (u32)MemPtr;

	Status = XUsbPs_ConfigureDevice(UsbInstancePtr, &DeviceConfig);
	if (XST_SUCCESS != Status)
	{
		goto out;
	}

	/* 设置接收帧的处理程序。 */
	Status = XUsbPs_IntrSetHandler(UsbInstancePtr, UsbIntrHandler, NULL,
								   XUSBPS_IXR_UE_MASK);
	if (XST_SUCCESS != Status)
	{
		goto out;
	}

	/* 设置处理端点0事件的处理程序。这是我们接收和处理来自主机的设置数据包的地方。
	 */
	Status = XUsbPs_EpSetHandler(UsbInstancePtr, 0,
								 XUSBPS_EP_DIRECTION_OUT,
								 XUsbPs_Ep0EventHandler, UsbInstancePtr);

	/* 设置处理端点1事件的处理程序（CDC 数据端点）。
	 *
	 * 注意在这个示例中我们为两个方向都注册处理程序，以便正确处理 CDC 数据通信。
	 */
	Status = XUsbPs_EpSetHandler(UsbInstancePtr, 1,
								 XUSBPS_EP_DIRECTION_OUT,
								 XUsbPs_Ep1EventHandler, UsbInstancePtr);

	Status = XUsbPs_EpSetHandler(UsbInstancePtr, 1,
								 XUSBPS_EP_DIRECTION_IN,
								 XUsbPs_Ep1EventHandler, UsbInstancePtr);

	/* 设置处理端点2事件的处理程序（CDC 通知端点）。 */
	Status = XUsbPs_EpSetHandler(UsbInstancePtr, 2,
								 XUSBPS_EP_DIRECTION_IN,
								 XUsbPs_Ep2EventHandler, UsbInstancePtr);

	/* 启用中断。 */
	XUsbPs_IntrEnable(UsbInstancePtr, XUSBPS_IXR_UR_MASK |
										  XUSBPS_IXR_UI_MASK);

	/* 启动USB引擎 */
	XUsbPs_Start(UsbInstancePtr);

	printf("ZYNQ7010 USB CDC-ACM Virtual Serial Port - Deep Compatibility Mode\r\n");
	printf("=================================================================\r\n");
	printf("Hardware: USB3320C-EZK PHY Controller\r\n");
	printf("Crystal: 24MHz, Power: 1.8V, Interface: ULPI\r\n");
	printf("Device Class: CDC (0x02) - Maximum Compatibility\r\n");
	printf("Vendor ID: 0x04B4 (Cypress), Product ID: 0x0008\r\n");
	printf("Endpoint Size: 64 bytes (USB 2.0 Full Speed Compatible)\r\n");
	printf("Power Mode: Bus Powered (100mA)\r\n");
	printf("Status: Starting USB enumeration debugging...\r\n");
	printf("=================================================================\r\n");
	printf("Waiting for USB host connection...\r\n");
	printf("Device will appear as a virtual serial port (COM port) on host.\r\n");
	printf("Connect USB cable and use serial terminal to communicate.\r\n\r\n");

	/* Keep the device running and responding to USB requests */
	while (1)
	{
		/* The device will respond to USB requests via interrupt handlers */
		/* Main processing is handled in the interrupt callbacks */
		usleep(1000000); /* Sleep for 1 second to reduce CPU usage */

		/* Print periodic status if device is connected */
		if (DeviceConnected)
		{
			printf("Device active - Interrupts: %d\r\n", NumIrqs);
		}
	}

	/* 设置返回代码以表示成功，并继续执行清理代码。
	 */
	ReturnStatus = XST_SUCCESS;

out:
	/* 清理。禁用中断和清除处理程序总是安全的，
	 * 即使它们尚未启用/设置。对于禁用中断子系统也是如此。
	 */
	XUsbPs_Stop(UsbInstancePtr);
	XUsbPs_IntrDisable(UsbInstancePtr, XUSBPS_IXR_ALL);
#ifndef SDT
	UsbDisableIntrSystem(IntcInstancePtr, UsbIntrId);
#else
	XDisconnectInterruptCntrl(UsbConfigPtr->IntrId,
							  UsbConfigPtr->IntrParent);
#endif
	(int)XUsbPs_IntrSetHandler(UsbInstancePtr, NULL, NULL, 0);

	/* 释放分配的内存。
	 */
	if (NULL != UsbInstancePtr->UserDataPtr)
	{
		free(UsbInstancePtr->UserDataPtr);
	}
	return ReturnStatus;
}

/*****************************************************************************/
/**
 *
 * 此函数是为USB驱动程序执行处理的处理程序。
 * 它从中断上下文中调用，因此应尽量减少执行的处理量。
 *
 * 此处理程序提供了如何处理USB中断的示例，
 * 是特定于应用程序的。
 *
 * @param	CallBackRef 是在调用回调函数时传回的上层回调引用。
 * @param 	Mask 是中断掩码。
 * @param	CallBackRef 是用户数据引用。
 *
 * @return
 * 		- XST_SUCCESS 如果成功
 * 		- XST_FAILURE 出错时
 *
 * @note	无。
 *
 ******************************************************************************/
static void UsbIntrHandler(void *CallBackRef, u32 Mask)
{
	/* 抑制未使用参数警告 */
	(void)CallBackRef;
	
	NumIrqs++;

	/* 检查USB复位 */
	if (Mask & XUSBPS_IXR_UR_MASK)
	{
		printf("检测到USB复位\r\n");
		DeviceConnected = 0;
	}

	/* 检查USB接口（枚举完成） */
	if (Mask & XUSBPS_IXR_UI_MASK)
	{
		printf("USB枚举完成，设备已连接\r\n");
		DeviceConnected = 1;
	}

	printf("收到USB中断 (次数: %d, 掩码: 0x%08lx)\r\n", NumIrqs, (unsigned long)Mask);
}

/*****************************************************************************/
/**
 * 此函数注册用于处理端点0（控制）的回调。
 *
 * 它从中断上下文中调用，因此应尽量减少执行的处理量。
 *
 *
 * @param	CallBackRef 是注册函数时传入的引用。
 * @param	EpNum 是发生事件的端点编号。
 * @param	EventType 是发生的事件类型。
 *
 * @return	无。
 *
 ******************************************************************************/
static void XUsbPs_Ep0EventHandler(void *CallBackRef, u8 EpNum,
								   u8 EventType, void *Data)
{
	/* 抑制未使用参数警告 */
	(void)Data;
	
	XUsbPs *InstancePtr;
	int Status;
	XUsbPs_SetupData SetupData;
	u8 *BufferPtr;
	u32 BufferLen;
	u32 Handle;

	Xil_AssertVoid(NULL != CallBackRef);

	InstancePtr = (XUsbPs *)CallBackRef;

	switch (EventType)
	{

	/* 处理在端点0上接收到的设置数据包。 */
	case XUSBPS_EP_EVENT_SETUP_DATA_RECEIVED:
		printf("在EP0上接收到设置数据包\r\n");
		Status = XUsbPs_EpGetSetupData(InstancePtr, EpNum, &SetupData);
		if (XST_SUCCESS == Status)
		{
			/* 处理设置数据包。 */
			(int)XUsbPs_Ch9HandleSetupPacket(InstancePtr,
											 &SetupData);
		}
		break;

	/* 对于端点0上的0长度数据包，我们获得数据RX事件。我们在这里
	 * 接收并立即再次释放它们，但不需要执行任何操作。
	 */
	case XUSBPS_EP_EVENT_DATA_RX:
		/* 获取数据缓冲区。 */
		Status = XUsbPs_EpBufferReceive(InstancePtr, EpNum,
										&BufferPtr, &BufferLen, &Handle);
		if (XST_SUCCESS == Status)
		{
			/* 返回缓冲区。 */
			XUsbPs_EpBufferRelease(Handle);
		}
		break;

	default:
		/* 未处理的事件。忽略。 */
		break;
	}
}

/*****************************************************************************/
/**
 * 此函数注册用于处理端点 1（批量数据）的回调。
 *
 * 它在中断上下文中被调用，因此应最大限度地减少执行的处理量。
 *
 * 此修改后的版本向主机发送“Hello World”消息。
 *
 * @param CallBackRef 是注册函数时传入的引用。
 * @param EpNum 是发生事件的端点的编号。
 * @param EventType 是发生的事件的类型。
 *
 * @return 无。
 *
 * @note 无。
 *
 ******************************************************************************/
static void XUsbPs_Ep1EventHandler(void *CallBackRef, u8 EpNum,
								   u8 EventType, void *Data)
{
	XUsbPs *InstancePtr;
	int Status;
	u8 *BufferPtr;
	u32 BufferLen;
	u32 InavalidateLen;
	u32 Handle;

	/* 要发送给主机的回显消息 */
	static int response_count = 0;

	Xil_AssertVoid(NULL != CallBackRef);

	InstancePtr = (XUsbPs *)CallBackRef;

	switch (EventType)
	{
	case XUSBPS_EP_EVENT_DATA_RX:
		printf("在 EP1 上接收到数据\r\n");

		/* 获取数据缓冲区。 */
		Status = XUsbPs_EpBufferReceive(InstancePtr, EpNum,
										&BufferPtr, &BufferLen, &Handle);

		if (XST_SUCCESS == Status)
		{
			/* 使缓冲区指针无效 */
			InavalidateLen = BufferLen;
			if (BufferLen % 32)
			{
				InavalidateLen = (BufferLen / 32) * 32 + 32;
			}

			Xil_DCacheInvalidateRange((unsigned int)BufferPtr,
									  InavalidateLen);

			/* 打印接收到的数据以进行调试 */
			printf("从主机接收到 %lu 字节数据: ", (unsigned long)BufferLen);
			for (u32 i = 0; i < BufferLen && i < 64; i++)
			{
				char c = BufferPtr[i];
				printf("%c", (c >= 32 && c <= 126) ? c : '.');
			}
			printf("\r\n");

			/* 将数据回显给主机（加上响应编号） */
			char response_msg[512];
			int prefix_len = sprintf(response_msg, "[Echo #%d] ", ++response_count);

			/* 复制接收到的数据到响应消息中 */
			int copy_len = (BufferLen < (u32)(512 - prefix_len - 1)) ? (int)BufferLen : (512 - prefix_len - 1);
			memcpy(response_msg + prefix_len, BufferPtr, (size_t)copy_len);
			response_msg[prefix_len + copy_len] = '\0';

			/* 发送回显响应 */
			Status = XUsbPs_EpBufferSend(InstancePtr, EpNum,
										 (u8 *)response_msg, prefix_len + copy_len);

			if (Status == XST_SUCCESS)
			{
				printf("已发送回显响应: %s", response_msg);
			}
			else
			{
				printf("发送响应失败，状态: %d\r\n", Status);
			}

			/* 释放接收到的缓冲区 */
			XUsbPs_EpBufferRelease(Handle);
		}
		else
		{
			printf("接收缓冲区失败，状态: %d\r\n", Status);
		}
		break;

	case XUSBPS_EP_EVENT_DATA_TX:
		printf("在 EP1 上已传输数据\r\n");
		break;

	default:
		printf("EP1 上未处理的事件: %d\r\n", EventType);
		break;
	}
}

/*****************************************************************************/
/**
 * 此函数注册用于处理端点 2（CDC 通知端点）的回调。
 *
 * 它在中断上下文中被调用，因此应最大限度地减少执行的处理量。
 *
 * 此端点用于发送 CDC 通知，如串行状态变化。
 *
 * @param CallBackRef 是注册函数时传入的引用。
 * @param EpNum 是发生事件的端点的编号。
 * @param EventType 是发生的事件的类型。
 *
 * @return 无。
 *
 * @note 无。
 *
 ******************************************************************************/
static void XUsbPs_Ep2EventHandler(void *CallBackRef, u8 EpNum,
								   u8 EventType, void *Data)
{
	/* 抑制未使用参数警告 */
	(void)EpNum;
	(void)EventType;
	(void)Data;
	
	XUsbPs *InstancePtr;

	Xil_AssertVoid(NULL != CallBackRef);

	InstancePtr = (XUsbPs *)CallBackRef;

	switch (EventType)
	{
	case XUSBPS_EP_EVENT_DATA_TX:
		printf("在 EP2 上已传输通知数据\r\n");
		break;

	default:
		printf("EP2 上未处理的事件: %d\r\n", EventType);
		break;
	}
}

/*****************************************************************************/
/**
 *
 * 此函数设置中断系统，以便 USB 控制器可以发生中断。此函数是特定于应用程序的，
 * 因为实际系统可能有也可能没有中断控制器。USB 控制器可以直接连接到没有中断控制器的处理器。
 * 用户应修改此函数以适应应用程序。
 *
 * @param IntcInstancePtr 是指向 Intc 控制器实例的指针。
 * @param UsbInstancePtr 是指向 USB 控制器实例的指针。
 * @param UsbIntrId 是中断 ID，通常是来自 xparameters.h 的
 * XPAR_<INTC_instance>_<USB_instance>_VEC_ID 值
 *
 * @return
 * - XST_SUCCESS（如果成功）
 * - XST_FAILURE（如果出错）
 *
 ******************************************************************************/
#ifndef SDT
static int UsbSetupIntrSystem(XScuGic *IntcInstancePtr,
							  XUsbPs *UsbInstancePtr, u16 UsbIntrId)
{
	int Status;
	XScuGic_Config *IntcConfig;

	/*
	 * 初始化中断控制器驱动程序，使其准备就绪可用。
	 */
	IntcConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
	if (NULL == IntcConfig)
	{
		return XST_FAILURE;
	}
	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
								   IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}

	Xil_ExceptionInit();
	/*
	 * 将中断控制器中断处理程序连接到处理器中的硬件中断处理逻辑。
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
								 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
								 IntcInstancePtr);
	/*
	 * 连接设备驱动程序处理程序，当设备发生中断时将调用该处理程序，
	 * 上面定义的处理程序执行设备的特定中断处理。
	 */
	Status = XScuGic_Connect(IntcInstancePtr, UsbIntrId,
							 (Xil_ExceptionHandler)XUsbPs_IntrHandler,
							 (void *)UsbInstancePtr);
	if (Status != XST_SUCCESS)
	{
		return Status;
	}
	/*
	 * 为设备启用中断。
	 */
	XScuGic_Enable(IntcInstancePtr, UsbIntrId);

	/*
	 * 在处理器中启用中断。
	 */
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
 *
 * 此函数禁用 USB 控制器发生的中断。
 *
 * @param IntcInstancePtr 是指向 INTC 驱动程序实例的指针。
 * @param UsbIntrId 是中断 ID，通常是来自 xparameters.h 的
 * XPAR_<INTC_instance>_<USB_instance>_VEC_ID 值
 *
 * @return 无
 *
 * @note 无。
 *
 ******************************************************************************/
static void UsbDisableIntrSystem(XScuGic *IntcInstancePtr, u16 UsbIntrId)
{
	/* 断开并禁用 USB 控制器的中断。 */
	XScuGic_Disconnect(IntcInstancePtr, UsbIntrId);
}
#endif
