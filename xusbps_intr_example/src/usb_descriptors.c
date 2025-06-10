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

// 临时定义CDC调用管理功能描述符结构体（解决编译问题）
typedef struct {
    uint8_t bFunctionLength;    // 描述符大小 (5)
    uint8_t bDescriptorType;    // CS_INTERFACE (0x24)
    uint8_t bDescriptorSubtype; // CALL_MGMT (0x01)
    uint8_t bmCapabilities;     // 能力
    uint8_t bDataInterface;     // 数据类接口的接口号
} __attribute__((__packed__)) USB_CDC_CALL_MGT_FND_DESC;

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

// CDC 端点号和大小 - 与main.c中的硬件配置保持一致
#define CDC_NOTIFICATION_EP 2 // 端点2用于中断通知 (IN)
#define CDC_DATA_OUT_EP 1	  // 端点1用于数据输出 (OUT from Host)
#define CDC_DATA_IN_EP 1	  // 端点1用于数据输入 (IN to Host)

#define CDC_NOTIFICATION_EPSIZE 16 // 中断端点大小
#define CDC_DATA_EPSIZE 64		   // Bulk 端点大小 (兼容性优先，自动协商高速)

// 完整配置描述符结构 - 添加IAD支持
typedef struct {
    USB_STD_CFG_DESC        config;
    // 接口关联描述符 (IAD) - 用于混合设备类
    struct {
        uint8_t bLength;                       // 8
        uint8_t bDescriptorType;              // 0x0B (IAD)
        uint8_t bFirstInterface;              // 0 (第一个接口)
        uint8_t bInterfaceCount;              // 2 (接口数量)
        uint8_t bFunctionClass;               // 0x02 (CDC)
        uint8_t bFunctionSubClass;            // 0x02 (ACM)
        uint8_t bFunctionProtocol;            // 0x01 (AT Commands)
        uint8_t iFunction;                    // 0 (no string)
    } iad;
    USB_STD_IF_DESC         if_comm;           // CDC通信接口
    USB_CDC_HEADER_FND_DESC cdc_header;        // CDC头部功能描述符
    USB_CDC_CALL_MGT_FND_DESC cdc_call_mgt;   // CDC调用管理功能描述符
    USB_CDC_ACM_FND_DESC    cdc_acm;          // CDC ACM功能描述符  
    USB_CDC_UNION_FND_DESC  cdc_union;        // CDC联合功能描述符
    USB_STD_EP_DESC         ep_comm;          // 通信端点(中断)
    USB_STD_IF_DESC         if_data;          // CDC数据接口
    USB_STD_EP_DESC         ep_data_out;      // 数据输出端点
    USB_STD_EP_DESC         ep_data_in;       // 数据输入端点
} __attribute__((packed)) COMPLETE_CDC_CONFIG_DESC;

// 完整配置描述符数据 - 包含IAD的Windows兼容配置
static const COMPLETE_CDC_CONFIG_DESC complete_config = {
    // 配置描述符
    {
        9,                                              // bLength
        0x02,                                          // bDescriptorType = Configuration
        75, 0x00,                                      // wTotalLength (little endian: 75) - 包含IAD
        2,                                             // bNumInterfaces = 2
        1,                                             // bConfigurationValue = 1
        0,                                             // iConfiguration = 0 (no string)
        0x80,                                          // bmAttributes = Bus powered
        50                                             // bMaxPower = 100mA (50 * 2mA)
    },
    
    // 接口关联描述符 (IAD) - Windows兼容性关键
    {
        8,              // bLength
        0x0B,           // bDescriptorType = Interface Association
        0,              // bFirstInterface = 0
        2,              // bInterfaceCount = 2
        0x02,           // bFunctionClass = CDC
        0x02,           // bFunctionSubClass = ACM
        0x01,           // bFunctionProtocol = AT Commands V.250
        0               // iFunction = 0 (no string)
    },
    
    // CDC通信接口描述符
    {
        9,              // bLength  
        0x04,           // bDescriptorType = Interface
        0,              // bInterfaceNumber = 0
        0,              // bAlternateSetting = 0
        1,              // bNumEndpoints = 1 (中断端点)
        0x02,           // bInterfaceClass = CDC Communication
        0x02,           // bInterfaceSubClass = ACM
        0x01,           // bInterfaceProtocol = AT Command V.250 (Windows标准)
        0               // iInterface = 0 (no string)
    },
    
    // CDC头部功能描述符
    {
        5,              // bFunctionLength
        0x24,           // bDescriptorType = CS_Interface  
        0x00,           // bDescriptorSubtype = Header
        0x10, 0x01      // bcdCDC = 1.10 (little endian: 0x0110)
    },
    
    // CDC调用管理功能描述符
    {
        5,              // bFunctionLength
        0x24,           // bDescriptorType = CS_Interface
        0x01,           // bDescriptorSubtype = Call Management
        0x03,           // bmCapabilities = 设备处理调用管理+使用数据接口
        1               // bDataInterface = 数据接口为接口1
    },
    
    // CDC ACM功能描述符
    {
        4,              // bFunctionLength
        0x24,           // bDescriptorType = CS_Interface
        0x02,           // bDescriptorSubtype = ACM
        0x06            // bmCapabilities = SET_LINE_CODING + GET_LINE_CODING + SET_CONTROL_LINE_STATE (Windows完整支持)
    },
    
    // CDC联合功能描述符  
    {
        5,              // bFunctionLength
        0x24,           // bDescriptorType = CS_Interface
        0x06,           // bDescriptorSubtype = Union
        0,              // bMasterInterface = 0
        1               // bSlaveInterface = 1
    },
    
    // 通信端点描述符(中断) - 修复端点地址匹配硬件
    {
        7,              // bLength
        0x05,           // bDescriptorType = Endpoint
        0x81,           // bEndpointAddress = IN endpoint 1 (修复为与硬件配置一致)
        0x03,           // bmAttributes = Interrupt
        0x08, 0x00,     // wMaxPacketSize = 8 (little endian, CDC标准)
        0x20            // bInterval = 32ms (更符合CDC标准)
    },
    
    // CDC数据接口描述符
    {
        9,              // bLength
        0x04,           // bDescriptorType = Interface  
        1,              // bInterfaceNumber = 1
        0,              // bAlternateSetting = 0
        2,              // bNumEndpoints = 2 (bulk in + bulk out)
        0x0A,           // bInterfaceClass = CDC Data
        0x00,           // bInterfaceSubClass = Unused
        0x00,           // bInterfaceProtocol = None
        0               // iInterface = 0 (no string)
    },
    
    // 数据输出端点(bulk out) - 修复端点地址
    {
        7,              // bLength
        0x05,           // bDescriptorType = Endpoint
        0x01,           // bEndpointAddress = OUT endpoint 1 (修复为与硬件配置一致)
        0x02,           // bmAttributes = Bulk
        0x40, 0x00,     // wMaxPacketSize = 64 (little endian)
        0               // bInterval = 0 (ignored for bulk)
    },
    
    // 数据输入端点(bulk in) - 修复端点地址
    {
        7,              // bLength
        0x05,           // bDescriptorType = Endpoint
        0x82,           // bEndpointAddress = IN endpoint 2 (修复为与硬件配置一致)
        0x02,           // bmAttributes = Bulk  
        0x40, 0x00,     // wMaxPacketSize = 64 (little endian)
        0               // bInterval = 0 (ignored for bulk)
    }
};

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
	// 极简设备描述符 - 使用混合设备类配置
	static const uint8_t minimal_device_desc[18] = {
		18,                         // bLength = 18
		0x01,                       // bDescriptorType = Device
		0x00, 0x02,                // bcdUSB = USB 2.0 (little endian)
		0xEF,                       // bDeviceClass = Miscellaneous (多接口设备)
		0x02,                       // bDeviceSubClass = Common Class
		0x01,                       // bDeviceProtocol = Interface Association Descriptor  
		64,                         // bMaxPacketSize0 = 64
		0x25, 0x09,                // idVendor = 0x0925 (Lakeview Research, CDC专用)
		0x67, 0x04,                // idProduct = 0x0467 (CDC Serial Port)
		0x00, 0x01,                // bcdDevice = 1.0 (little endian)
		0x01,                       // iManufacturer = String index 1
		0x02,                       // iProduct = String index 2  
		0x03,                       // iSerialNumber = String index 3
		0x01                        // bNumConfigurations = 1
	};

	uint32_t XUsbPs_Ch9SetupDevDescReply(uint8_t *BufPtr, uint32_t BufLen)
	{
		printf("MINIMAL Device Descriptor Request: BufLen=%d\r\n", (int)BufLen);
		
		if (!BufPtr || BufLen < 18) {
			printf("ERROR: Insufficient buffer for device descriptor\r\n");
			return 0;
		}
		
		memcpy(BufPtr, minimal_device_desc, 18);
		
		printf("MINIMAL Device Descriptor sent: 18 bytes\r\n");
		printf("VID:PID = 0x0925:0x0467, Class=0xEF (Miscellaneous/CDC-ACM)\r\n");
		
		return 18;
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
		printf("COMPLETE Config Descriptor Request: BufLen=%d\r\n", (int)BufLen);
		
		if (!BufPtr || BufLen < sizeof(COMPLETE_CDC_CONFIG_DESC)) {
			printf("ERROR: Insufficient buffer for config descriptor\r\n");
			return 0;
		}
		
		memcpy(BufPtr, &complete_config, sizeof(COMPLETE_CDC_CONFIG_DESC));
		
		printf("COMPLETE Config Descriptor sent: %d bytes\r\n", 
			   (int)sizeof(COMPLETE_CDC_CONFIG_DESC));
		printf("Interfaces: 2 (CDC Control + Data), Endpoints: 3, With IAD + Call Management\r\n");
		
		return sizeof(COMPLETE_CDC_CONFIG_DESC);
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
		// 添加字符串描述符调试信息
		printf("String Descriptor Request: Index=%d, BufLen=%d\r\n", Index, (int)BufLen);

		if (!BufPtr)
		{
			printf("ERROR: String descriptor buffer pointer is NULL\r\n");
			return 0;
		}

		// 使用静态预构建的字符串描述符以避免动态构造问题
		static const uint8_t StringDesc0[] = {
			4,          // bLength
			0x03,       // bDescriptorType (STRING)
			0x09, 0x04  // wLANGID[0] = 0x0409 (English US)
		};

		static const uint8_t StringDesc1[] = {
			12,         // bLength
			0x03,       // bDescriptorType (STRING) 
			'X', 0, 'i', 0, 'l', 0, 'i', 0, 'n', 0, 'x', 0  // "Xilinx" in UTF-16LE
		};

		static const uint8_t StringDesc2[] = {
			24,         // bLength
			0x03,       // bDescriptorType (STRING)
			'U', 0, 'S', 0, 'B', 0, ' ', 0, 'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0, 0, 0  // "USB Device" in UTF-16LE
		};

		static const uint8_t StringDesc3[] = {
			22,         // bLength  
			0x03,       // bDescriptorType (STRING)
			'V', 0, 'C', 0, 'O', 0, 'M', 0, '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, 0, 0  // "VCOM12345" in UTF-16LE
		};

		static const uint8_t StringDesc4[] = {
			18,         // bLength
			0x03,       // bDescriptorType (STRING)
			'C', 0, 'D', 0, 'C', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0, 'f', 0  // "CDC Conf" in UTF-16LE
		};

		static const uint8_t StringDesc5[] = {
			16,         // bLength
			0x03,       // bDescriptorType (STRING)
			'C', 0, 'D', 0, 'C', 0, ' ', 0, 'A', 0, 'C', 0, 'M', 0  // "CDC ACM" in UTF-16LE
		};

		const uint8_t *desc_ptr = NULL;
		uint32_t desc_len = 0;

		switch (Index)
		{
		case 0:
			desc_ptr = StringDesc0;
			desc_len = sizeof(StringDesc0);
			printf("String Descriptor 0 (Language ID): Length=%d\r\n", (int)desc_len);
			break;
		case 1:
			desc_ptr = StringDesc1;
			desc_len = sizeof(StringDesc1);
			printf("String Descriptor 1 (Manufacturer): Length=%d\r\n", (int)desc_len);
			break;
		case 2:
			desc_ptr = StringDesc2;
			desc_len = sizeof(StringDesc2);
			printf("String Descriptor 2 (Product): Length=%d\r\n", (int)desc_len);
			break;
		case 3:
			desc_ptr = StringDesc3;
			desc_len = sizeof(StringDesc3);
			printf("String Descriptor 3 (Serial): Length=%d\r\n", (int)desc_len);
			break;
		case 4:
			desc_ptr = StringDesc4;
			desc_len = sizeof(StringDesc4);
			printf("String Descriptor 4 (Configuration): Length=%d\r\n", (int)desc_len);
			break;
		case 5:
			desc_ptr = StringDesc5;
			desc_len = sizeof(StringDesc5);
			printf("String Descriptor 5 (Function): Length=%d\r\n", (int)desc_len);
			break;
		default:
			printf("ERROR: String index %d not supported\r\n", Index);
			return 0;
		}

		/* 检查提供的缓冲区是否足够大以容纳描述符。 */
		if (desc_len > BufLen)
		{
			printf("ERROR: String descriptor buffer too small: %d > %d\r\n", 
				   (int)desc_len, (int)BufLen);
			return 0;
		}

		memcpy(BufPtr, desc_ptr, desc_len);
		
		printf("String Descriptor %d sent successfully: %d bytes\r\n", 
			   Index, (int)desc_len);

		return desc_len;
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
