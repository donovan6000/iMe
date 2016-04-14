// Header gaurd
#ifndef CONF_USB_H
#define CONF_USB_H


// Header files
#include "compiler.h"


// Device definitions
#define USB_DEVICE_VENDOR_ID USB_VID_ATMEL
#define USB_DEVICE_PRODUCT_ID USB_PID_ATMEL_ASF_CDC
#define USB_DEVICE_MAJOR_VERSION 1
#define USB_DEVICE_MINOR_VERSION 0
#define USB_DEVICE_POWER 100
#define USB_DEVICE_ATTR USB_CONFIG_ATTR_SELF_POWERED


// Device string definitions
#define USB_DEVICE_MANUFACTURE_NAME "M3D"
#define USB_DEVICE_PRODUCT_NAME "The Micro"
#define USB_DEVICE_SERIAL_NAME
#define USB_DEVICE_GET_SERIAL_NAME_POINTER serialNumber
#define USB_DEVICE_GET_SERIAL_NAME_LENGTH 16
extern char serialNumber[];


// High speed support
#if(UC3A3 || UC3A4)
	#define USB_DEVICE_HS_SUPPORT
#elif(SAM3XA || SAM3U)
	#define USB_DEVICE_HS_SUPPORT
#endif


// USB interrupt priority
#define UDD_USB_INT_LEVEL USB_INTLVL_LO_gc


// Communication ports used
#define UDI_CDC_PORT_NB 1


// Communication port configuration
#define UDI_CDC_LOW_RATE
#define UDI_CDC_DEFAULT_RATE 115200
#define UDI_CDC_DEFAULT_STOPBITS CDC_STOP_BITS_1
#define UDI_CDC_DEFAULT_PARITY CDC_PAR_NONE
#define UDI_CDC_DEFAULT_DATABITS 8


// CDC callbacks
#define UDI_CDC_ENABLE_EXT(port) true
#define UDI_CDC_DISABLE_EXT(port)
#define UDI_CDC_RX_NOTIFY(port) cdcRxNotifyCallback(port)
extern void cdcRxNotifyCallback(uint8_t port);
#define UDI_CDC_TX_EMPTY_NOTIFY(port)
#define UDI_CDC_SET_CODING_EXT(port, cfg)
#define UDI_CDC_SET_DTR_EXT(port, set)
#define UDI_CDC_SET_RTS_EXT(port, set)


// Header files
#include "udi_cdc_conf.h"


#endif
