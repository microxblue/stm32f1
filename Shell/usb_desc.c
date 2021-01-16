/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
* File Name          : usb_desc.c
* Author             : MCD Application Team
* Version            : V3.3.0
* Date               : 21-March-2011
* Description        : Descriptors for Virtual Com Port Demo
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "usb_lib.h"
#include "usb_desc.h"

/* USB Standard Device Descriptor */
uint8_t Device_DeviceDescriptor[USB_DEIVCE_SIZ_DEVICE_DESC] =
  {
    0x12,   /* bLength */
    USB_DEVICE_DESCRIPTOR_TYPE,     /* bDescriptorType */
    0x00,
    0x02,   /* bcdUSB = 2.00 */
    0xEF,   /* bDeviceClass:  */
    0x02,   /* bDeviceSubClass */
    0x01,   /* bDeviceProtocol */
    0x40,   /* bMaxPacketSize0 */
    0x86,
    0x06,   /* idVendor = 0x0483 -> 0xffff added by david halter -> undefined */
    0x23,   /* in make file */
    0x10,   /* idProduct = 0x5740 */
    0x00,
    0x02,   /* bcdDevice = 2.00 */
    1,      /* Index of string descriptor describing manufacturer */
    2,      /* Index of string descriptor describing product */
    3,      /* Index of string descriptor describing the device's serial number */
    0x01    /* bNumConfigurations */
  };

//size: USB_DEIVCE_SIZ_CONFIG_DESC !!!!!
const uint8_t Device_ConfigDescriptor[USB_DEIVCE_SIZ_CONFIG_DESC] =
  {
    /*Configuration Descriptor*/
    0x09,   /* bLength: Configuration Descriptor size */
    USB_CONFIGURATION_DESCRIPTOR_TYPE,      /* bDescriptorType: Configuration */
    USB_DEIVCE_SIZ_CONFIG_DESC,       /* wTotalLength:no of returned bytes */
    0x00,
    0x02,   /* bNumInterfaces: 2 interface */
    0x01,   /* bConfigurationValue: Configuration value */
    0x00,   /* iConfiguration: Index of string descriptor describing the configuration */
    0xC0,   /* bmAttributes: self powered */
    0x32,   /* MaxPower 0 mA */

    /********************  Interface 1 ********************/
    0x09,   /* bLength: Interface Descriptor size */
    USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: Interface */
    /* Interface descriptor type */
    0x00,   // bInterfaceNumber: Number of Interface
    0x00,   // bAlternateSetting: Alternate setting
    0x02,   // bNumEndpoints: One endpoints used
    0xFF,   // bInterfaceClass: HID -> 03
    0xFf,   // bInterfaceSubClass : 1=BOOT, 0=no boot
    0xFF,   // nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse
    0x00,   // iInterface:

    /********************  Endpoints 0 ********************/
    0x07,   /*Endpoint descriptor length = 7*/
    0x05,   /*Endpoint descriptor type */
    EP_CON_IN,   /*Endpoint address (IN, address 1) */
    0x02,   /*Bulk endpoint type */
    EP_MAX_SIZE,   /*wMaxPacketSize (LSB) */
    0x00,   /*wMaxPacketSize (MSB) */
    0x00,   /*Polling interval in milliseconds */

    /********************  Endpoints 1 ********************/
    0x07,   /*Endpoint descriptor length = 7 */
    0x05,   /*Endpoint descriptor type */
    EP_CON_OUT,   /*Endpoint address (OUT, address 1) */
    0x02,   /*Bulk endpoint type */
    EP_MAX_SIZE,   /*wMaxPacketSize (LSB) */
    0x00,   /*wMaxPacketSize (MSB) */
    0x00,   /*Polling interval in milliseconds*/

    /********************  Interface 2 ********************/
    0x09,   /* bLength: Endpoint Descriptor size */
    USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: */
    0x01,   /* bInterfaceNumber: Number of Interface */
    0x00,   /* bAlternateSetting: Alternate setting */
    0x02,   /* bNumEndpoints: Two endpoints used */
    0xFF,   /* bInterfaceClass: CDC */
    0xFF,   /* bInterfaceSubClass: */
    0xFF,   /* bInterfaceProtocol: */
    0x00,   /* iInterface: */

    /********************  Endpoints 0 ********************/
    0x07,   /*Endpoint descriptor length = 7*/
    0x05,   /*Endpoint descriptor type */
    EP_COMM_IN,   /*Endpoint address (IN, address 1) */
    0x02,   /*Bulk endpoint type */
    EP_MAX_SIZE,   /*wMaxPacketSize (LSB) */
    0x00,   /*wMaxPacketSize (MSB) */
    0x00,   /*Polling interval in milliseconds */

    /********************  Endpoints 1 ********************/
    0x07,   /*Endpoint descriptor length = 7 */
    0x05,   /*Endpoint descriptor type */
    EP_COMM_OUT,   /*Endpoint address (OUT, address 1) */
    0x02,   /*Bulk endpoint type */
    EP_MAX_SIZE,   /*wMaxPacketSize (LSB) */
    0x00,   /*wMaxPacketSize (MSB) */
    0x00    /*Polling interval in milliseconds*/

  };

/* USB String Descriptors */
const uint8_t Device_StringLangID[USB_DEIVCE_SIZ_STRING_LANGID] =
  {
    USB_DEIVCE_SIZ_STRING_LANGID,
    USB_STRING_DESCRIPTOR_TYPE,
    0x09,
    0x04 /* LangID = 0x0409: U.S. English */
  };

const uint8_t Device_StringVendor[USB_DEIVCE_SIZ_STRING_VENDOR] =
  {
    USB_DEIVCE_SIZ_STRING_VENDOR,     /* Size of Vendor string */
    USB_STRING_DESCRIPTOR_TYPE,             /* bDescriptorType*/
    /* Manufacturer: "STMicroelectronics" */
    'M', 0, 'X', 0, 'M', 0, 'A', 0
  };

const uint8_t Device_StringProduct[USB_DEIVCE_SIZ_STRING_PRODUCT] =
  {
    USB_DEIVCE_SIZ_STRING_PRODUCT,          /* bLength */
    USB_STRING_DESCRIPTOR_TYPE,        /* bDescriptorType */
    /* Product name: "LED controller" */
    'S', 0, 'T', 0, 'M', 0, ' ', 0,
#if USER_FLAGS == 1
    'U', 0, 'S', 0, 'B', 0, ' ', 0,
#else
    'D', 0, 'e', 0, 'v', 0, ' ', 0,
#endif
    'B', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0
  };

uint8_t Device_StringSerial[USB_DEIVCE_SIZ_STRING_SERIAL] =
  {
    USB_DEIVCE_SIZ_STRING_SERIAL,           /* bLength */
    USB_STRING_DESCRIPTOR_TYPE,                   /* bDescriptorType */
    'S', 0, 'T', 0, 'M', 0, '3', 0, '2', 0, '1', 0, '0', 0
  };

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
