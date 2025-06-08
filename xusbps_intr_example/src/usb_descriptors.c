/*****************************************************************************/
/**
 * @file xusbps_ch9_storage.c
 *
 * 此文件包含示例的存储特定第 9 章代码的实现。
 *
 *<pre>
 * 修改历史:
 *
 * 版本  谁   日期     更改
 * ----- ---- -------- ---------------------------------------------------------
 * 1.00a wgr  10/10/10 首次发布
 * 2.5	 pm   02/20/20 添加了 SetConfigurationApp 和 SetInterfaceHandler API 以
 *			使 ch9 成为所有示例的通用框架。
 *</pre>
 ******************************************************************************/

/***************************** 包含文件 *********************************/

#include <string.h>

#include "xparameters.h" /* XPAR parameters */
#include "xusbps.h"		 /* USB controller driver */

#include "usb_ch9_handler.h"
#include "usb_descriptors.h"

/************************** 常量定义 *****************************/

/***************** 宏 (内联函数) 定义 *********************/

/**************************** 类型定义 *******************************/

#ifdef __ICCARM__
#pragma pack(push, 1)
#endif
typedef struct
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;
	uint8_t bNumConfigurations;
#ifdef __ICCARM__
} USB_STD_DEV_DESC;
#pragma pack(pop)
#else
} __attribute__((__packed__)) USB_STD_DEV_DESC;
#endif

/* 使用头文件中定义的结构体，不在这里重复定义 */

/************************** 函数原型 ******************************/

/************************** 变量定义 *****************************/

#define USB_ENDPOINT0_MAXP 0x40

#define USB_BULKIN_EP 1
#define USB_BULKOUT_EP 1

#define USB_DEVICE_DESC 0x01
#define USB_CONFIG_DESC 0x02
#define USB_STRING_DESC 0x03
#define USB_INTERFACE_CFG_DESC 0x04
#define USB_ENDPOINT_CFG_DESC 0x05

// CDC 端点号和大小
#define CDC_NOTIFICATION_EP 2 // 端点2用于中断通知 (IN)
#define CDC_DATA_OUT_EP 1	  // 端点1用于数据输出 (OUT from Host)
#define CDC_DATA_IN_EP 1	  // 端点1用于数据输入 (IN to Host)

#define CDC_NOTIFICATION_EPSIZE 16 // 中断端点大小
#define CDC_DATA_EPSIZE 64		   // Bulk 端点大小 (兼容性优先，自动协商高速)

	/*****************************************************************************/
	/**
	 *
	 * 此函数返回设备的设备描述符。
	 *
	 * @param BufPtr 是指向要用描述符填充的缓冲区的指针。
	 * @param BufLen 是提供的缓冲区的大小。
	 *
	 * @return 成功时缓冲区中描述符的长度。
	 *		出错时为 0。
	 *
	 ******************************************************************************/
	uint32_t XUsbPs_Ch9SetupDevDescReply(uint8_t *BufPtr, uint32_t BufLen)
	{
		USB_STD_DEV_DESC deviceDesc = {
			sizeof(USB_STD_DEV_DESC), /* bLength */
			USB_DEVICE_DESC,		  /* bDescriptorType */
			be2les(0x0200),			  /* bcdUSB 2.0 */
			0x02,					  /* bDeviceClass - CDC Communication Device Class */
			0x00,					  /* bDeviceSubClass - Unused at device level */
			0x00,					  /* bDeviceProtocol - Unused at device level */
			USB_ENDPOINT0_MAXP,		  /* bMaxPacketSize0 */
			be2les(0x04B4),			  /* idVendor - Cypress (兼容性测试) */
			be2les(0x0008),			  /* idProduct - CDC设备 */
			be2les(0x0100),			  /* bcdDevice (版本 1.0) */
			0x01,					  /* iManufacturer */
			0x02,					  /* iProduct */
			0x03,					  /* iSerialNumber */
			0x01					  /* bNumConfigurations */
		};
		
		// 添加调试信息
		printf("Device Descriptor Request: BufLen=%d, DescSize=%d\r\n", 
			   (int)BufLen, (int)sizeof(USB_STD_DEV_DESC));
		printf("Device Class: CDC (0x02), VID:PID = 0x04B4:0x0008\r\n");

		/* 检查缓冲区指针是否存在以及缓冲区是否足够大。 */
		if (!BufPtr)
		{
			printf("ERROR: Device descriptor buffer pointer is NULL\r\n");
			return 0;
		}

		if (BufLen < sizeof(USB_STD_DEV_DESC))
		{
			printf("ERROR: Device descriptor buffer too small: %d < %d\r\n", 
				   (int)BufLen, (int)sizeof(USB_STD_DEV_DESC));
			return 0;
		}

		memcpy(BufPtr, &deviceDesc, sizeof(USB_STD_DEV_DESC));
		
		printf("Device Descriptor sent successfully: %d bytes\r\n", 
			   (int)sizeof(USB_STD_DEV_DESC));

		return sizeof(USB_STD_DEV_DESC);
	}

	/*****************************************************************************/
	/**
	 *
	 * 此函数返回设备的配置描述符。
	 *
	 * @param	BufPtr 是指向要用描述符填充的缓冲区的指针。
	 * @param	BufLen 是提供的缓冲区的大小。
	 *
	 * @return 	成功时缓冲区中描述符的长度。
	 *		出错时为 0。
	 *
	 ******************************************************************************/
	uint32_t XUsbPs_Ch9SetupCfgDescReply(uint8_t *BufPtr, uint32_t BufLen)
	{
		// 使用简化的 CDC 配置描述符结构（不使用IAD以提高兼容性）
		USB_CDC_CONFIG_DESC_FULL config = {
			/* 标准配置描述符 */
			{
				sizeof(USB_STD_CFG_DESC),				  /* bLength */
				USB_CONFIG_DESC,						  /* bDescriptorType */
				be2les(sizeof(USB_CDC_CONFIG_DESC_FULL)), /* wTotalLength */
				0x02,									  /* bNumInterfaces (CDC Control + CDC Data) */
				0x01,									  /* bConfigurationValue */
				0x04,									  /* iConfiguration */
				0x80,									  /* bmAttributes (Bus Powered, 不支持远程唤醒) */
				0x32									  /* bMaxPower (100mA - 降低功耗要求) */
			},

			/* 接口关联描述符 (IAD) - 暂时保留但简化 */
			{
				sizeof(USB_STD_IAD_DESC),		   /* bLength */
				XUSBPS_TYPE_INTERFACE_ASSOCIATION, /* bDescriptorType (0x0B) */
				0x00,							   /* bFirstInterface */
				0x02,							   /* bInterfaceCount */
				XUSBPS_CLASS_CDC,				   /* bFunctionClass */
				XUSBPS_CDC_SUBCLASS_ACM,		   /* bFunctionSubClass */
				0x00,							   /* bFunctionProtocol (无特定协议) */
				0x05							   /* iFunction */
			},

			/* CDC 控制接口描述符 */
			{
				sizeof(USB_STD_IF_DESC),	/* bLength */
				USB_INTERFACE_CFG_DESC,		/* bDescriptorType */
				0x00,						/* bInterfaceNumber */
				0x00,						/* bAlternateSetting */
				0x01,						/* bNumEndPoints */
				XUSBPS_CLASS_CDC,			/* bInterfaceClass */
				XUSBPS_CDC_SUBCLASS_ACM,	/* bInterfaceSubClass */
				0x00,						/* bInterfaceProtocol (简化协议) */
				0x06						/* iInterface */
			},

			/* CDC Header Functional Descriptor */
			{
				sizeof(USB_CDC_HEADER_FND_DESC), /* bFunctionLength */
				XUSBPS_TYPE_CS_INTERFACE,		 /* bDescriptorType (CS_INTERFACE) */
				XUSBPS_CDC_FND_HEADER,			 /* bDescriptorSubtype (HEADER) */
				be2les(0x0110)					 /* bcdCDC (CDC Spec version 1.10) */
			},

			/* CDC Call Management Functional Descriptor */
			{
				sizeof(USB_CDC_CALL_MGMT_FND_DESC), /* bFunctionLength */
				XUSBPS_TYPE_CS_INTERFACE,			/* bDescriptorType */
				XUSBPS_CDC_FND_CALL_MGMT,			/* bDescriptorSubtype */
				0x01,								/* bmCapabilities (D0: Device handles call mgmt) */
				0x01								/* bDataInterface */
			},

			/* CDC Abstract Control Management Functional Descriptor */
			{
				sizeof(USB_CDC_ACM_FND_DESC), /* bFunctionLength */
				XUSBPS_TYPE_CS_INTERFACE,	  /* bDescriptorType */
				XUSBPS_CDC_FND_ACM,			  /* bDescriptorSubtype */
				0x06						  /* bmCapabilities (SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE) */
			},

			/* CDC Union Functional Descriptor */
			{
				sizeof(USB_CDC_UNION_FND_DESC), /* bFunctionLength */
				XUSBPS_TYPE_CS_INTERFACE,		/* bDescriptorType */
				XUSBPS_CDC_FND_UNION,			/* bDescriptorSubtype (UNION) */
				0x00,							/* bMasterInterface (Interface number of control interface) */
				0x01							/* bSlaveInterface0 (Interface number of data interface) */
			},

			/* CDC 通知端点描述符 (Interrupt IN) */
			{
				sizeof(USB_STD_EP_DESC),		 /* bLength */
				USB_ENDPOINT_CFG_DESC,			 /* bDescriptorType */
				0x80 | CDC_NOTIFICATION_EP,		 /* bEndpointAddress (IN endpoint, EP number 2) */
				0x03,							 /* bmAttributes (Interrupt) */
				be2les(CDC_NOTIFICATION_EPSIZE), /* wMaxPacketSize */
				0xFF							 /* bInterval (FS: 255ms, HS: polling interval for interrupt EPs) */
			},

			/* CDC 数据接口描述符 */
			{
				sizeof(USB_STD_IF_DESC), /* bLength */
				USB_INTERFACE_CFG_DESC,	 /* bDescriptorType */
				0x01,					 /* bInterfaceNumber */
				0x00,					 /* bAlternateSetting */
				0x02,					 /* bNumEndPoints (Bulk IN & Bulk OUT) */
				XUSBPS_CLASS_CDC_DATA,	 /* bInterfaceClass (CDC Data) */
				0x00,					 /* bInterfaceSubClass (Not used) */
				0x00,					 /* bInterfaceProtocol (Not used) */
				0x07					 /* iInterface (字符串描述符索引 for "CDC Data") */
			},

			/* CDC 数据输出端点描述符 (Bulk OUT) */
			{
				sizeof(USB_STD_EP_DESC), /* bLength */
				USB_ENDPOINT_CFG_DESC,	 /* bDescriptorType */
				CDC_DATA_OUT_EP,		 /* bEndpointAddress (OUT endpoint, EP number 1) */
				0x02,					 /* bmAttributes (Bulk) */
				be2les(CDC_DATA_EPSIZE), /* wMaxPacketSize */
				0x00					 /* bInterval (Ignored for Bulk) */
			},

			/* CDC 数据输入端点描述符 (Bulk IN) */
			{
				sizeof(USB_STD_EP_DESC), /* bLength */
				USB_ENDPOINT_CFG_DESC,	 /* bDescriptorType */
				0x80 | CDC_DATA_IN_EP,	 /* bEndpointAddress (IN endpoint, EP number 1) */
				0x02,					 /* bmAttributes (Bulk) */
				be2les(CDC_DATA_EPSIZE), /* wMaxPacketSize */
				0x00					 /* bInterval (Ignored for Bulk) */
			}};

	/* 检查缓冲区指针是否正常以及缓冲区是否足够大。 */
	if (!BufPtr)
	{
		printf("ERROR: Config descriptor buffer pointer is NULL\r\n");
		return 0;
	}

	// 注意：这里应该比较 BufLen 和 sizeof(USB_CDC_CONFIG_DESC_FULL)
	if (BufLen < sizeof(USB_CDC_CONFIG_DESC_FULL))
	{
		printf("WARNING: Config descriptor buffer small: %d < %d\r\n", 
			   (int)BufLen, (int)sizeof(USB_CDC_CONFIG_DESC_FULL));
		// 如果缓冲区太小，可以考虑只复制 BufLen 字节
		// 或者返回错误。标准的做法是返回0或实际复制的字节数。
		// 为简单起见，如果缓冲区不足，我们目前返回0。
		// 在实际应用中，主机通常会请求足够的长度。
		return 0;
	}

	// 添加配置描述符调试信息
	printf("Config Descriptor Request: BufLen=%d, ConfigSize=%d\r\n", 
		   (int)BufLen, (int)sizeof(USB_CDC_CONFIG_DESC_FULL));

	memcpy(BufPtr, &config, sizeof(USB_CDC_CONFIG_DESC_FULL));
	
	printf("Config Descriptor sent successfully: %d bytes\r\n", 
		   (int)sizeof(USB_CDC_CONFIG_DESC_FULL));

	return sizeof(USB_CDC_CONFIG_DESC_FULL);
	}

	/*****************************************************************************/
	/**
	 *
	 * 此函数返回给定索引的字符串描述符。
	 *
	 * @param\tBufPtr 是指向要用描述符填充的缓冲区的指针。
	 * @param\tBufLen 是提供的缓冲区的大小。
	 * @param\tIndex 是请求描述符的字符串的索引。
	 *
	 * @return \t成功时缓冲区中描述符的长度。
	 *\t\t出错时为 0。
	 *
	 ******************************************************************************/
	uint32_t XUsbPs_Ch9SetupStrDescReply(uint8_t *BufPtr, uint32_t BufLen, uint8_t Index)
	{
		int i;

		// 更新字符串列表以包含 CDC 所需的字符串
		static char *StringList[] = {
			"UNUSED",					 // 0: LANGID (特殊处理)
			"Xilinx",					 // 1: iManufacturer
			"ZYNQ7010 Virtual COM Port", // 2: iProduct
			"VCOM12345678",				 // 3: iSerialNumber
			"CDC Configuration",		 // 4: iConfiguration
			"CDC ACM Interface",		 // 5: iFunction (for IAD), and also for CDC Control Interface
			"CDC Control Interface",	 // 6: iInterface (for CDC Control, if different from iFunction)
			"CDC Data Interface",		 // 7: iInterface (for CDC Data)
		};
		char *String;
		uint32_t StringLen;
		uint32_t DescLen;
		uint8_t TmpBuf[128];

		USB_STD_STRING_DESC *StringDesc;

		if (!BufPtr)
		{
			return 0;
		}

		if (Index >= sizeof(StringList) / sizeof(char *))
		{
			return 0;
		}

		String = StringList[Index];
		StringLen = strlen(String);

		StringDesc = (USB_STD_STRING_DESC *)TmpBuf;

		/* 索引 0 是特殊的，因为我们无法在上面的表中表示所需的字符串。
		 * 因此，我们将索引 0 作为特殊情况处理。
		 */
		if (0 == Index)
		{
			StringDesc->bLength = 4;
			StringDesc->bDescriptorType = USB_STRING_DESC;
			StringDesc->wString[0] = be2les(0x0409);
		}
		/* 所有其他字符串都可以从上面的表中提取。 */
		else
		{
			StringDesc->bLength = StringLen * 2 + 2;
			StringDesc->bDescriptorType = USB_STRING_DESC;

			for (i = 0; i < StringLen; i++)
			{
				StringDesc->wString[i] = be2les((uint16_t)String[i]);
			}
		}
		DescLen = StringDesc->bLength;

		/* 检查提供的缓冲区是否足够大以容纳描述符。 */
		if (DescLen > BufLen)
		{
			return 0;
		}

		memcpy(BufPtr, StringDesc, DescLen);

		return DescLen;
	}

	/*****************************************************************************/
	/**
	 * 此函数处理“设置配置”请求。
	 *
	 * @param	InstancePtr 是指向控制器的 XUsbPs 实例的指针。
	 * @param	ConfigIdx 是所需配置的索引。
	 *
	 * @return	无
	 *
	 ******************************************************************************/
	void XUsbPs_SetConfiguration(XUsbPs *InstancePtr, int ConfigIdx)
	{
		Xil_AssertVoid(InstancePtr != NULL);
		uint32_t RegVal;

		/* 我们只有一个配置。它的索引是 1。忽略其他任何配置。
		 */
		if (1 != ConfigIdx)
		{
			// 也许应该发送一个 STALL 给主机？
			// 或者根据 USB 规范禁用所有非 EP0 端点。
			// 目前，我们只返回。
			return;
		}

		/* 启用 CDC 端点 */
		// CDC 通知端点 (中断 IN, EP2)
		XUsbPs_EpEnable(InstancePtr, CDC_NOTIFICATION_EP, XUSBPS_EP_DIRECTION_IN);
		// CDC 数据端点 (Bulk OUT & IN, EP1)
		XUsbPs_EpEnable(InstancePtr, CDC_DATA_OUT_EP, XUSBPS_EP_DIRECTION_OUT);
		XUsbPs_EpEnable(InstancePtr, CDC_DATA_IN_EP, XUSBPS_EP_DIRECTION_IN);

		/* 配置端点类型 */
		// EP1: Bulk OUT 和 Bulk IN
		// XUSBPS_EPCR_TXR_MASK 和 XUSBPS_EPCR_RXR_MASK 用于复位数据切换
		RegVal = XUsbPs_ReadReg(InstancePtr, XUSBPS_EPCR1_OFFSET + (CDC_DATA_OUT_EP * XUSBPS_EPCR_OFFSET));
		RegVal &= ~(XUSBPS_EPCR_RXT_MASK | XUSBPS_EPCR_TXT_MASK); // 清除类型位
		RegVal |= (XUSBPS_EPCR_RXT_BULK_MASK | XUSBPS_EPCR_TXT_BULK_MASK | XUSBPS_EPCR_TXR_MASK | XUSBPS_EPCR_RXR_MASK);
		XUsbPs_WriteReg(InstancePtr, XUSBPS_EPCR1_OFFSET + (CDC_DATA_OUT_EP * XUSBPS_EPCR_OFFSET), RegVal);

		// EP2: Interrupt IN
		// 注意：Xilinx USB 控制器似乎没有为每个端点单独的 EPCR 寄存器，
		// EPCR0 用于 EP0, EPCR1 用于 EP1, EPCR2 用于 EP2, ... EPCRn 用于 EPn
		// 所以我们需要访问 XUSBPS_EPCR1_OFFSET + (CDC_NOTIFICATION_EP * XUSBPS_EPCR_OFFSET)
		// 但是，XUSBPS_EPCR_OFFSET 定义为 0。这看起来像是驱动程序/硬件的一个特性，
		// 即 EPCR[0] 控制 EP0，EPCR[1] 控制 EP1，EPCR[2] 控制 EP2，等等。
		// XUSBPS_EPCR_OFFSET 应该是每个 EPCR 寄存器之间的偏移量，如果它们是连续的。
		// 查阅 Zynq TRM (UG585) 的 USB Device Controller 章节，
		// 端点配置寄存器 (EPCRx, x=0-11) 位于偏移量 0x100 + 4*x。
		// XUSBPS_EPCR0_OFFSET 是 0x100。所以 EPCRn 的地址是 XUSBPS_EPCR0_OFFSET + 4*n。
		// XUSBPS_EPCR1_OFFSET 在 xusbps_hw.h 中定义为 0x104。
		// 因此，EPCR[n] 的地址应该是 XUSBPS_EPCR0_OFFSET + (n * 4)。

		RegVal = XUsbPs_ReadReg(InstancePtr, XUSBPS_EPCR0_OFFSET + (CDC_NOTIFICATION_EP * 4));
		RegVal &= ~XUSBPS_EPCR_TXT_MASK;							  // 清除 TX 类型位
		RegVal |= (XUSBPS_EPCR_TXT_INTR_MASK | XUSBPS_EPCR_TXR_MASK); // 设置为 Interrupt IN，复位数据切换
		// RX 方向对于 IN 端点不适用，但为了安全，可以清除或设置为禁用
		RegVal &= ~XUSBPS_EPCR_RXT_MASK;
		RegVal |= XUSBPS_EPCR_RXT_DISABLE_MASK; // 或者不设置 RX，让其保持默认/禁用
		XUsbPs_WriteReg(InstancePtr, XUSBPS_EPCR0_OFFSET + (CDC_NOTIFICATION_EP * 4), RegVal);

		/* 预先准备好 OUT 端点。 */
		// CDC 数据 OUT 端点 (EP1 OUT)
		XUsbPs_EpPrime(InstancePtr, CDC_DATA_OUT_EP, XUSBPS_EP_DIRECTION_OUT);

		// CDC 通知 IN 端点 (EP2 IN) - 中断端点通常在有数据要发送时才 Prime
		// XUsbPs_EpPrime(InstancePtr, CDC_NOTIFICATION_EP, XUSBPS_EP_DIRECTION_IN);
		// CDC 数据 IN 端点 (EP1 IN) - Bulk IN 端点通常在有数据要发送时才 Prime
		// XUsbPs_EpPrime(InstancePtr, CDC_DATA_IN_EP, XUSBPS_EP_DIRECTION_IN);
	}

	/****************************************************************************/
	/**
	 * 当从主机接收到 SET_CONFIGURATION 命令时，此函数由 Chapter9 处理程序调用。
	 *
	 * @param	InstancePtr 是指向控制器的 XUsbPs 实例的指针。
	 * @param	SetupData 是从主机接收到的设置数据包。
	 *
	 * @return
	 *		- XST_SUCCESS 如果成功，
	 *		- XST_FAILURE 如果不成功。
	 *
	 * @note
	 *		必须在 SET_CONFIGURATION 命令之后启用非控制端点
	 *		因为硬件会清除此命令接收时所有先前启用的端点
	 *		控制端点除外。
	 *
	 *****************************************************************************/
	void XUsbPs_SetConfigurationApp(XUsbPs *InstancePtr,
									XUsbPs_SetupData *SetupData)
	{
		(void)InstancePtr;
		(void)SetupData;
	}

	/****************************************************************************/
	/**
	 * 当从主机接收到 SET_CONFIGURATION 命令时，此函数由 Chapter9 处理程序调用
	 * 或 SET_INTERFACE 命令。
	 *
	 * @param	InstancePtr 是指向控制器的 XUsbPs 实例的指针。
	 * @param	SetupData 是从主机接收到的设置数据包。
	 *
	 * @note
	 *
	 *****************************************************************************/
	void XUsbPs_SetInterfaceHandler(XUsbPs *InstancePtr,
									XUsbPs_SetupData *SetupData)
	{
		(void)InstancePtr;
		(void)SetupData;
	}
