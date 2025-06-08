/******************************************************************************
 * 版权所有 (C) 2010 - 2022 Xilinx, Inc. 保留所有权利。
 * 版权所有 (C) 2023 - 2024 Advanced Micro Devices, Inc. 保留所有权利。
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

/*****************************************************************************/
/**
 * @file xusbps_ch9.c
 *
 * 此文件包含示例的第 9 章代码的实现。
 *
 *<pre>
 * 修改历史:
 *
 * 版本  谁   日期     更改
 * ----- ---- -------- ---------------------------------------------------------
 * 1.00a jz  10/10/10 首次发布
 * 1.04a nm  02/05/13 修复了 CR# 696550。
 *		      添加了供应商请求的模板代码。
 * 1.04a nm  03/04/13 修复了 CR# 704022。实现了 TEST_MODE 功能。
 * 1.06a kpc 11/11/13 DMA 操作始终使用全局内存
 * 2.1   kpc 4/29/14  将 DMA 缓冲区与缓存行边界对齐
 * 2.4	 vak 4/01/19  修复了 IAR data_alignment 警告
 * 2.9   nd  3/18/24  修复了 CV 测试套件报告的故障。
 *</pre>
 ******************************************************************************/

/***************************** 包含文件 *********************************/

#include "xparameters.h" /* XPAR parameters */
#include "xusbps.h"		 /* USB controller driver */
#include "xusbps_hw.h"	 /* USB controller driver */

#include "usb_ch9_handler.h"
#include "usb_cdc_class.h" // 包含 CDC 类处理器
#include "xil_printf.h"
#include "xil_cache.h"

#include "sleep.h"

/* #define CH9_DEBUG */

#ifdef CH9_DEBUG
#include <stdio.h>
#define printf xil_printf
#endif

/************************** 常量定义 *****************************/
/**************************** 类型定义 *******************************/

/***************** 宏 (内联函数) 定义 *********************/

/************************** 函数原型 ******************************/

static void XUsbPs_StdDevReq(XUsbPs *InstancePtr,
							 XUsbPs_SetupData *SetupData);

static int XUsbPs_HandleVendorReq(XUsbPs *InstancePtr,
								  XUsbPs_SetupData *SetupData);
// 使用 CDC 类处理器而不是通用的 ClassReq
// extern void XUsbPs_ClassReq(XUsbPs *InstancePtr,
//			    XUsbPs_SetupData *SetupData);
extern u32 XUsbPs_Ch9SetupDevDescReply(u8 *BufPtr, u32 BufLen);
extern u32 XUsbPs_Ch9SetupCfgDescReply(u8 *BufPtr, u32 BufLen);
extern u32 XUsbPs_Ch9SetupStrDescReply(u8 *BufPtr, u32 BufLen, u8 Index);
extern void XUsbPs_SetConfiguration(XUsbPs *InstancePtr, int ConfigIdx);
extern void XUsbPs_SetConfigurationApp(XUsbPs *InstancePtr,
									   XUsbPs_SetupData *SetupData);
extern void XUsbPs_SetInterfaceHandler(XUsbPs *InstancePtr,
									   XUsbPs_SetupData *SetupData);

/************************** 变量定义 *****************************/

#ifdef __ICCARM__
#pragma data_alignment = 32
static u8 Response;
#else
static u8 Response ALIGNMENT_CACHELINE;
#endif

/*****************************************************************************/
/**
 * 此函数处理来自主机的设置数据包。
 *
 * @param	InstancePtr 是指向控制器 XUsbPs 实例的指针。
 * @param	SetupData 是包含设置请求的结构。
 *
 * @return
 *		- XST_SUCCESS 如果函数成功。
 *		- XST_FAILURE 如果发生错误。
 *
 * @note		无。
 *
 ******************************************************************************/
int XUsbPs_Ch9HandleSetupPacket(XUsbPs *InstancePtr,
								XUsbPs_SetupData *SetupData)
{
	int Status = XST_SUCCESS;

#ifdef CH9_DEBUG
	printf("Handle setup packet\n");
#endif

	switch (SetupData->bmRequestType & XUSBPS_REQ_TYPE_MASK)
	{
	case XUSBPS_CMD_STDREQ:
		XUsbPs_StdDevReq(InstancePtr, SetupData);
		break;

	case XUSBPS_CMD_CLASSREQ:
		// 使用 CDC 类处理器
		Status = XUsbPs_Cdc_ClassReq(InstancePtr, SetupData);
		if (Status != XST_SUCCESS)
		{
			// CDC 处理器返回失败，STALL 端点0
			XUsbPs_EpStall(InstancePtr, 0, XUSBPS_EP_DIRECTION_IN | XUSBPS_EP_DIRECTION_OUT);
		}
		break;

	case XUSBPS_CMD_VENDREQ:

#ifdef CH9_DEBUG
		printf("供应商请求 %x\n", SetupData->bRequest);
#endif
		Status = XUsbPs_HandleVendorReq(InstancePtr, SetupData);
		break;

	default:
		/* 在端点 0 上停止 */
#ifdef CH9_DEBUG
		printf("未知的类请求，在输出中停止 0\n");
#endif
		XUsbPs_EpStall(InstancePtr, 0, XUSBPS_EP_DIRECTION_IN | XUSBPS_EP_DIRECTION_OUT);
		break;
	}

	return Status;
}

/*****************************************************************************/
/**
 * 此函数处理标准设备请求。
 *
 * @param	InstancePtr 是指向控制器 XUsbPs 实例的指针。
 * @param	SetupData 是指向包含设置请求的数据结构的指针。
 *
 * @return	无。
 *
 * @note		无。
 *
 ******************************************************************************/
static void XUsbPs_StdDevReq(XUsbPs *InstancePtr,
							 XUsbPs_SetupData *SetupData)
{
	int Status;
	int Error = 0;
	u32 Handler;
	u32 TmpBufferLen = 6;
	u8 *TempPtr;
	XUsbPs_Local *UsbLocalPtr;

	int ReplyLen;
#ifdef __ICCARM__
#pragma data_alignment = 32
	static u8 Reply[XUSBPS_REQ_REPLY_LEN];
	static u8 TmpBuffer[10];
#else
	static u8 Reply[XUSBPS_REQ_REPLY_LEN] ALIGNMENT_CACHELINE;
	static u8 TmpBuffer[10] ALIGNMENT_CACHELINE;
#endif

	TempPtr = (u8 *)&TmpBuffer;

	/* 检查请求的回复长度是否大于我们的回复缓冲区。
	 * 这应该永远不会发生...
	 */
	if (SetupData->wLength > XUSBPS_REQ_REPLY_LEN)
	{
		return;
	}

	UsbLocalPtr = (XUsbPs_Local *)InstancePtr->UserDataPtr;

#ifdef CH9_DEBUG
	printf("std dev req %d\n", SetupData->bRequest);
#endif

	switch (SetupData->bRequest)
	{

	case XUSBPS_REQ_GET_STATUS:

		switch (SetupData->bmRequestType & XUSBPS_STATUS_MASK)
		{
		case XUSBPS_STATUS_DEVICE:
			/* 即使我们只使用前两个字节，
			 * 似乎我们也不必担心将回复缓冲区的其余部分清零。
			 */
			*((u16 *)&Reply[0]) = 0x1; /* 自供电 */
			break;

		case XUSBPS_STATUS_INTERFACE:
			*((u16 *)&Reply[0]) = 0x0;
			break;

		case XUSBPS_STATUS_ENDPOINT:
		{
			u32 Status;
			int EpNum = SetupData->wIndex;

			Status = XUsbPs_ReadReg(InstancePtr->Config.BaseAddress,
									XUSBPS_EPCRn_OFFSET(EpNum & 0xF));

			if (EpNum & 0x80)
			{ /* In EP */
				if (Status & XUSBPS_EPCR_TXS_MASK)
				{
					*((u16 *)&Reply[0]) = 1;
				}
				else
				{
					*((u16 *)&Reply[0]) = 0;
				}
			}
			else
			{ /* Out EP */
				if (Status & XUSBPS_EPCR_RXS_MASK)
				{
					*((u16 *)&Reply[0]) = 1;
				}
				else
				{
					*((u16 *)&Reply[0]) = 0;
				}
			}
			break;
		}

		default:;
#ifdef CH9_DEBUG
			printf("未知的状态请求 %x\n", SetupData->bmRequestType);
#endif
		}
		XUsbPs_EpBufferSend(InstancePtr, 0, Reply, SetupData->wLength);
		break;

	case XUSBPS_REQ_SET_ADDRESS:

		/* 设置位 24 后，地址值将保存在影子寄存器中，
		 * 直到状态阶段被确认。此时，
		 * 地址值将写入地址寄存器。
		 */
		XUsbPs_SetDeviceAddress(InstancePtr, SetupData->wValue);
#ifdef CH9_DEBUG
		printf("设置地址 %d\n", SetupData->wValue);
#endif
		/* 没有数据阶段，因此通过发送零长度数据包来确认事务。
		 */
		XUsbPs_EpBufferSend(InstancePtr, 0, NULL, 0);
		break;

	case XUSBPS_REQ_GET_INTERFACE:
#ifdef CH9_DEBUG
		printf("获取接口 %d/%d/%d\n",
			   SetupData->wIndex, SetupData->wLength,
			   InstancePtr->CurrentAltSetting);
#endif
		Response = (u8)InstancePtr->CurrentAltSetting;

		/* 确认主机 */
		XUsbPs_EpBufferSend(InstancePtr, 0, &Response, 1);

		break;

	case XUSBPS_REQ_GET_DESCRIPTOR:
#ifdef CH9_DEBUG
		printf("获取描述符 %x/%d\n", (SetupData->wValue >> 8) & 0xff,
			   SetupData->wLength);
#endif

		/* 获取描述符类型。 */
		switch ((SetupData->wValue >> 8) & 0xff)
		{

		case XUSBPS_TYPE_DEVICE_DESC:
		case XUSBPS_TYPE_DEVICE_QUALIFIER:

			/* 使用设备描述符数据设置回复缓冲区。
			 */
			ReplyLen = XUsbPs_Ch9SetupDevDescReply(
				Reply, XUSBPS_REQ_REPLY_LEN);

			ReplyLen = ReplyLen > SetupData->wLength ? SetupData->wLength : ReplyLen;

			if (((SetupData->wValue >> 8) & 0xff) ==
				XUSBPS_TYPE_DEVICE_QUALIFIER)
			{
				Reply[0] = (u8)ReplyLen;
				Reply[1] = (u8)0x6;
				Reply[2] = (u8)0x0;
				Reply[3] = (u8)0x2;
				Reply[4] = (u8)0xFF;
				Reply[5] = (u8)0x00;
				Reply[6] = (u8)0x0;
				Reply[7] = (u8)0x10;
				Reply[8] = (u8)0;
				Reply[9] = (u8)0x0;
			}
			Status = XUsbPs_EpBufferSend(InstancePtr, 0,
										 Reply, ReplyLen);
			if (XST_SUCCESS != Status)
			{
				/* 需要处理故障情况 */
				for (;;)
					;
			}
			break;

		case XUSBPS_TYPE_CONFIG_DESC:

			/* 使用配置描述符数据设置回复缓冲区。
			 */
			ReplyLen = XUsbPs_Ch9SetupCfgDescReply(
				Reply, XUSBPS_REQ_REPLY_LEN);

#ifdef CH9_DEBUG
			printf("获取配置 %d/%d\n", ReplyLen, SetupData->wLength);
#endif

			ReplyLen = ReplyLen > SetupData->wLength ? SetupData->wLength : ReplyLen;

			Status = XUsbPs_EpBufferSend(InstancePtr, 0,
										 Reply, ReplyLen);
			if (XST_SUCCESS != Status)
			{
				/* 需要处理故障情况 */
				for (;;)
					;
			}
			break;

		case XUSBPS_TYPE_STRING_DESC:

			/* 使用字符串描述符数据设置回复缓冲区。
			 */
			ReplyLen = XUsbPs_Ch9SetupStrDescReply(
				Reply, XUSBPS_REQ_REPLY_LEN,
				SetupData->wValue & 0xFF);

			ReplyLen = ReplyLen > SetupData->wLength ? SetupData->wLength : ReplyLen;

			Status = XUsbPs_EpBufferSend(InstancePtr, 0,
										 Reply, ReplyLen);
			if (XST_SUCCESS != Status)
			{
				/* 需要处理故障情况 */
				for (;;)
					;
			}
			break;

#ifdef MOUSE_SIMULATION
		case XUSBPS_TYPE_HID_DESC:

			/* 使用 HID 描述符数据设置回复缓冲区。
			 */
			ReplyLen = XUsbPs_Ch9SetupHidDescReply(
				Reply, XUSBPS_REQ_REPLY_LEN);

			ReplyLen = ReplyLen > SetupData->wLength ? SetupData->wLength : ReplyLen;

			Status = XUsbPs_EpBufferSend(InstancePtr, 0,
										 Reply, ReplyLen);
			if (XST_SUCCESS != Status)
			{
				/* 需要处理故障情况 */
				for (;;)
					;
			}
			break;

		case XUSBPS_TYPE_REPORT_DESC:

			/* 使用报告描述符数据设置回复缓冲区。
			 */
			ReplyLen = XUsbPs_Ch9SetupReportDescReply(
				Reply, XUSBPS_REQ_REPLY_LEN);
#ifdef CH9_DEBUG
			printf("报告描述符长度 %d\n", ReplyLen);
#endif

			ReplyLen = ReplyLen > SetupData->wLength ? SetupData->wLength : ReplyLen;

			Status = XUsbPs_EpBufferSend(InstancePtr, 0,
										 Reply, ReplyLen);
			if (XST_SUCCESS != Status)
			{
				/* 需要处理故障情况 */
				for (;;)
					;
			}
			break;
#endif /* MOUSE_SIMULATION */

		default:
			Error = 1;
			break;
		}
		break;

	case XUSBPS_REQ_SET_CONFIGURATION:

		/*
		 *  允许配置索引 0 和 1。
		 */
		if (((SetupData->wValue & 0xff) != 1) && ((SetupData->wValue & 0xff) != 0))
		{
			Error = 1;
			break;
		}

		UsbLocalPtr->CurrentConfig = SetupData->wValue & 0xff;

		/* 调用特定于应用程序的配置函数以应用具有给定配置索引的配置。
		 */
		XUsbPs_SetConfiguration(InstancePtr,
								UsbLocalPtr->CurrentConfig);

		// 如果配置不为0，初始化 CDC 类处理器
		if (UsbLocalPtr->CurrentConfig != 0)
		{
			XUsbPs_Cdc_Init(InstancePtr);
		}

		if (InstancePtr->AppData != NULL)
		{
			XUsbPs_SetConfigurationApp(InstancePtr, SetupData);
		}

		/* 没有数据阶段，因此通过发送零长度数据包来确认事务。
		 */
		XUsbPs_EpBufferSend(InstancePtr, 0, NULL, 0);
		break;

	case XUSBPS_REQ_GET_CONFIGURATION:

		XUsbPs_EpBufferSend(InstancePtr, 0,
							&UsbLocalPtr->CurrentConfig, 1);
		break;

	case XUSBPS_REQ_CLEAR_FEATURE:
		switch (SetupData->bmRequestType & XUSBPS_STATUS_MASK)
		{
		case XUSBPS_STATUS_ENDPOINT:
			if (SetupData->wValue == XUSBPS_ENDPOINT_HALT)
			{
				int EpNum = SetupData->wIndex;

				if (EpNum & 0x80)
				{ /* 输入端点 */
					XUsbPs_ClrBits(InstancePtr,
								   XUSBPS_EPCRn_OFFSET(EpNum & 0xF),
								   XUSBPS_EPCR_TXS_MASK);
				}
				else
				{ /* 输出端点 */
					XUsbPs_ClrBits(InstancePtr,
								   XUSBPS_EPCRn_OFFSET(EpNum),
								   XUSBPS_EPCR_RXS_MASK);
				}
			}
			/* 确认主机？ */
			XUsbPs_EpBufferSend(InstancePtr, 0, NULL, 0);
			break;

		default:
			Error = 1;
			break;
		}

		break;

	case XUSBPS_REQ_SET_FEATURE:
		switch (SetupData->bmRequestType & XUSBPS_STATUS_MASK)
		{
		case XUSBPS_STATUS_ENDPOINT:
			if (SetupData->wValue == XUSBPS_ENDPOINT_HALT)
			{
				int EpNum = SetupData->wIndex;

				if (EpNum & 0x80)
				{ /* 输入端点 */
					XUsbPs_SetBits(InstancePtr,
								   XUSBPS_EPCRn_OFFSET(EpNum & 0xF),
								   XUSBPS_EPCR_TXS_MASK);
				}
				else
				{ /* 输出端点 */
					XUsbPs_SetBits(InstancePtr,
								   XUSBPS_EPCRn_OFFSET(EpNum),
								   XUSBPS_EPCR_RXS_MASK);
				}
			}
			/* 确认主机？ */
			XUsbPs_EpBufferSend(InstancePtr, 0, NULL, 0);

			break;
		case XUSBPS_STATUS_DEVICE:
			if (SetupData->wValue == XUSBPS_TEST_MODE)
			{
				int TestSel = (SetupData->wIndex >> 8) & 0xFF;

				/* 确认主机，转换必须在状态阶段之后且 < 3ms 内发生
				 */
				XUsbPs_EpBufferSend(InstancePtr, 0, NULL, 0);
				usleep(1000);

				switch (TestSel)
				{
				case XUSBPS_TEST_J:
				case XUSBPS_TEST_K:
				case XUSBPS_TEST_SE0_NAK:
				case XUSBPS_TEST_PACKET:
				case XUSBPS_TEST_FORCE_ENABLE:
					XUsbPs_SetBits(InstancePtr,
								   XUSBPS_PORTSCR1_OFFSET,
								   TestSel << 16);
					break;
				default:
					/* 不支持的测试选择器 */
					break;
				}
				break;
			}
			/* 如果不是测试模式，fallthrough 到默认处理 */
			/* fall through */

		default:
			Error = 1;
			break;
		}

		break;

	/* 对于设置接口，检查主机想要的备用设置 */
	case XUSBPS_REQ_SET_INTERFACE:

#ifdef CH9_DEBUG
		printf("设置接口 %d/%d\n", SetupData->wValue, SetupData->wIndex);
#endif
		/* 仅支持 ISO */
		if (InstancePtr->AppData != NULL)
			XUsbPs_SetInterfaceHandler(
				(XUsbPs *)InstancePtr, SetupData);

		/* 设备完成操作后确认主机 */
		Error = XUsbPs_EpBufferSend(InstancePtr, 0, NULL, 0);
		if (Error)
		{
#ifdef CH9_DEBUG
			printf("EpBufferSend failed %d\n", Error);
#endif
		}
		break;
	case XUSBPS_REQ_SET_SEL:
#ifdef CH9_DEBUG
		printf("设置选择 \r\n\r");
#endif

		Status = XUsbPs_EpBufferReceive((XUsbPs *)InstancePtr,
										0, &TempPtr, &TmpBufferLen, &Handler);
		if (XST_SUCCESS == Status)
		{
			/* 返回缓冲区。 */
			XUsbPs_EpBufferRelease(Handler);
		}

		break;

	case XUSBPS_REQ_SET_ISOCH_DELAY:
#ifdef CH9_DEBUG
		printf("设置 ISOCH 延迟 \r\n\r");
#endif
		break;

	default:
		Error = 1;
		break;
	}

	/* 如果发生错误，则设置发送停止位 */
	if (Error)
	{
#ifdef CH9_DEBUG
		printf("标准设备请求 %d/%d 错误，在输出中停止 0\n",
			   SetupData->bRequest, (SetupData->wValue >> 8) & 0xff);
#endif
		XUsbPs_EpStall(InstancePtr, 0, XUSBPS_EP_DIRECTION_IN | XUSBPS_EP_DIRECTION_OUT);
	}
}

/*****************************************************************************/
/**
 * 此函数处理供应商请求。
 *
 * @param	InstancePtr 是指向控制器 XUsbPs 实例的指针。
 * @param	SetupData 是指向包含设置请求的数据结构的指针。
 *
 * @return
 *		- XST_SUCCESS 如果成功。
 *		- XST_FAILURE 如果发生错误。
 *
 * @note
 *		此函数是处理控制
 *		IN 和控制 OUT 端点的供应商请求的模板。控制 OUT 端点
 *		每个 dTD 最多只能接收 64 字节的数据。要在控制 OUT 端点接收
 *		超过 64 字节的供应商数据，请更改控制 OUT 端点的缓冲区大小。
 *		否则，结果是不可预期的。
 *
 ******************************************************************************/
static int XUsbPs_HandleVendorReq(XUsbPs *InstancePtr,
								  XUsbPs_SetupData *SetupData)
{
	u8 *BufferPtr;
	u32 BufferLen;
	u32 Handle;
	u32 Reg;
#ifdef __ICCARM__
#pragma data_alignment = 32
	static const u8 Reply[8] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
#else
	static const u8 Reply[8] ALIGNMENT_CACHELINE =
		{0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
#endif
	u8 EpNum = 0;
	int Status;
	int Direction;
	int Timeout;

	/* 检查方向，USB 2.0 第 9.3 节 */
	Direction = SetupData->bmRequestType & (1 << 7);

	if (!Direction)
	{
		/* 控制 OUT 供应商请求 */
		if (SetupData->wLength > 0)
		{
			/* 重新准备端点以接收设置数据 */
			XUsbPs_EpPrime(InstancePtr, 0, XUSBPS_EP_DIRECTION_OUT);

			/* 检查 EP 准备是否成功 */
			Timeout = XUSBPS_TIMEOUT_COUNTER;
			do
			{
				Reg = XUsbPs_ReadReg(InstancePtr->Config.BaseAddress,
									 XUSBPS_EPPRIME_OFFSET);
			} while (((Reg & (1 << EpNum)) == 1) && --Timeout);

			if (!Timeout)
			{
				return XST_FAILURE;
			}

			/* 获取设置数据，不要等待中断 */
			Timeout = XUSBPS_TIMEOUT_COUNTER;
			do
			{
				Status = XUsbPs_EpBufferReceive(InstancePtr,
												EpNum, &BufferPtr, &BufferLen, &Handle);
			} while ((Status != XST_SUCCESS) && --Timeout);

			if (!Timeout)
			{
				return XST_FAILURE;
			}

			Xil_DCacheInvalidateRange((unsigned int)BufferPtr,
									  BufferLen);
#ifdef CH9_DEBUG
			int Len;
			xil_printf("供应商数据：\r\n");
			for (Len = 0; Len < BufferLen; Len++)
			{
				xil_printf("%02x ", BufferPtr[Len]);
			}
#endif

			if (Status == XST_SUCCESS)
			{
				/* 零长度确认 */
				Status = XUsbPs_EpBufferSend(InstancePtr, EpNum,
											 NULL, 0);
				if (Status != XST_SUCCESS)
				{
					/* 需要处理故障情况 */
					for (;;)
						;
				}
			}
		}
	}
	else
	{
		/* 控制 IN 供应商请求 */

		/* 检查请求的回复长度是否大于我们的回复缓冲区。
		 * 这应该永远不会发生...
		 */
		if (SetupData->wLength > sizeof(Reply))
		{
			return XST_FAILURE;
		}

		/* 发送固定的供应商回复数据。
		 */
		Status = XUsbPs_EpBufferSend(InstancePtr, 0, (u8 *)&Reply,
									 sizeof(Reply));
		if (Status != XST_SUCCESS)
		{
			/* 需要处理故障情况 */
			for (;;)
				;
		}
	}

	return XST_SUCCESS;
}
