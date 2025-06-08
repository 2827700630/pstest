//===================================================================
// 极简USB描述符 - 用于解决设备描述符请求失败问题
// 这是最基础的USB CDC设备实现，去除所有可能导致兼容性问题的配置
//===================================================================

// 如果当前的USB描述符仍然失败，请使用此极简版本替换

// 极简设备描述符
static const USB_STD_DEV_DESC minimal_device_desc = {
    sizeof(USB_STD_DEV_DESC),   // bLength = 18
    0x01,                       // bDescriptorType = Device
    0x0002,                     // bcdUSB = USB 2.0 (little endian: 0x0200)
    0x02,                       // bDeviceClass = CDC
    0x00,                       // bDeviceSubClass = Unused
    0x00,                       // bDeviceProtocol = Unused  
    64,                         // bMaxPacketSize0 = 64
    0xB404,                     // idVendor = 0x04B4 (Cypress, little endian)
    0x0800,                     // idProduct = 0x0008 (little endian)
    0x0001,                     // bcdDevice = 1.0 (little endian: 0x0100)
    0x01,                       // iManufacturer = String index 1
    0x02,                       // iProduct = String index 2  
    0x03,                       // iSerialNumber = String index 3
    0x01                        // bNumConfigurations = 1
};

// 极简配置描述符 (最基础的CDC实现)
typedef struct {
    USB_STD_CFG_DESC        config;
    USB_STD_IF_DESC         if_comm;           // CDC通信接口
    USB_CDC_HEADER_FND_DESC cdc_header;        // CDC头部功能描述符
    USB_CDC_ACM_FND_DESC    cdc_acm;          // CDC ACM功能描述符  
    USB_CDC_UNION_FND_DESC  cdc_union;        // CDC联合功能描述符
    USB_STD_EP_DESC         ep_comm;          // 通信端点(中断)
    USB_STD_IF_DESC         if_data;          // CDC数据接口
    USB_STD_EP_DESC         ep_data_out;      // 数据输出端点
    USB_STD_EP_DESC         ep_data_in;       // 数据输入端点
} __attribute__((packed)) MINIMAL_CDC_CONFIG_DESC;

static const MINIMAL_CDC_CONFIG_DESC minimal_config = {
    // 配置描述符
    {
        9,                                              // bLength
        0x02,                                          // bDescriptorType = Configuration
        sizeof(MINIMAL_CDC_CONFIG_DESC),               // wTotalLength (little endian)
        2,                                             // bNumInterfaces = 2
        1,                                             // bConfigurationValue = 1
        0,                                             // iConfiguration = 0 (no string)
        0x80,                                          // bmAttributes = Bus powered
        50                                             // bMaxPower = 100mA (50 * 2mA)
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
        0x00,           // bInterfaceProtocol = None
        0               // iInterface = 0 (no string)
    },
    
    // CDC头部功能描述符
    {
        5,              // bFunctionLength
        0x24,           // bDescriptorType = CS_Interface  
        0x00,           // bDescriptorSubtype = Header
        0x0110          // bcdCDC = 1.10 (little endian: 0x1001)
    },
    
    // CDC ACM功能描述符
    {
        4,              // bFunctionLength
        0x24,           // bDescriptorType = CS_Interface
        0x02,           // bDescriptorSubtype = ACM
        0x02            // bmCapabilities = SET_LINE_CODING supported
    },
    
    // CDC联合功能描述符  
    {
        5,              // bFunctionLength
        0x24,           // bDescriptorType = CS_Interface
        0x06,           // bDescriptorSubtype = Union
        0,              // bMasterInterface = 0
        1               // bSlaveInterface = 1
    },
    
    // 通信端点描述符(中断)
    {
        7,              // bLength
        0x05,           // bDescriptorType = Endpoint
        0x82,           // bEndpointAddress = IN endpoint 2
        0x03,           // bmAttributes = Interrupt
        16,             // wMaxPacketSize = 16 (little endian: 0x0010)
        0xFF            // bInterval = 255ms
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
    
    // 数据输出端点(bulk out)
    {
        7,              // bLength
        0x05,           // bDescriptorType = Endpoint
        0x01,           // bEndpointAddress = OUT endpoint 1  
        0x02,           // bmAttributes = Bulk
        64,             // wMaxPacketSize = 64 (little endian: 0x0040)
        0               // bInterval = 0 (ignored for bulk)
    },
    
    // 数据输入端点(bulk in)
    {
        7,              // bLength
        0x05,           // bDescriptorType = Endpoint
        0x81,           // bEndpointAddress = IN endpoint 1
        0x02,           // bmAttributes = Bulk  
        64,             // wMaxPacketSize = 64 (little endian: 0x0040)
        0               // bInterval = 0 (ignored for bulk)
    }
};

//===================================================================
// 替换函数 - 如果需要使用极简版本，请用以下函数替换原来的函数
//===================================================================

/*
uint32_t XUsbPs_Ch9SetupDevDescReply_Minimal(uint8_t *BufPtr, uint32_t BufLen)
{
    printf("MINIMAL Device Descriptor Request: BufLen=%d\r\n", (int)BufLen);
    
    if (!BufPtr || BufLen < sizeof(USB_STD_DEV_DESC)) {
        printf("ERROR: Insufficient buffer for device descriptor\r\n");
        return 0;
    }
    
    memcpy(BufPtr, &minimal_device_desc, sizeof(USB_STD_DEV_DESC));
    
    printf("MINIMAL Device Descriptor sent: 18 bytes\r\n");
    printf("VID:PID = 0x04B4:0x0008, Class=0x02\r\n");
    
    return sizeof(USB_STD_DEV_DESC);
}

uint32_t XUsbPs_Ch9SetupCfgDescReply_Minimal(uint8_t *BufPtr, uint32_t BufLen)  
{
    printf("MINIMAL Config Descriptor Request: BufLen=%d\r\n", (int)BufLen);
    
    if (!BufPtr || BufLen < sizeof(MINIMAL_CDC_CONFIG_DESC)) {
        printf("ERROR: Insufficient buffer for config descriptor\r\n");
        return 0;
    }
    
    memcpy(BufPtr, &minimal_config, sizeof(MINIMAL_CDC_CONFIG_DESC));
    
    printf("MINIMAL Config Descriptor sent: %d bytes\r\n", 
           (int)sizeof(MINIMAL_CDC_CONFIG_DESC));
    
    return sizeof(MINIMAL_CDC_CONFIG_DESC);
}
*/

//===================================================================
// 使用说明:
// 1. 如果当前的USB描述符仍然失败
// 2. 请将上述极简函数取消注释  
// 3. 在usb_descriptors.c中调用这些极简版本
// 4. 这将提供最基础的USB CDC设备实现
//===================================================================
