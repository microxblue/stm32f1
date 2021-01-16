/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
* File Name          : usb_desc.h
* Author             : MCD Application Team
* Version            : V3.3.0
* Date               : 21-March-2011
* Description        : Descriptor Header for Virtual COM Port Device
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_DESC_H
#define __USB_DESC_H

#define EP_CON_OUT     0x01
#define EP_CON_IN      0x81

#define EP_COMM_OUT    0x02
#define EP_COMM_IN     0x83

#define EP_MAX_SIZE    0x40

#define NUM_INTERFACE  2
#define NUM_ENDPOINT   4

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported define -----------------------------------------------------------*/
#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05

#define USB_OUT_DATA_SIZE                       0x40 //720 = 2D0
#define USB_DEIVCE_DATA_SIZE              64
#define USB_DEIVCE_INT_SIZE               8

#define HID_DESCRIPTOR_TYPE                     0x21
#define CUSTOMHID_SIZ_HID_DESC                  0x09
#define CUSTOMHID_OFF_HID_DESC                  0x12

#define USB_DEIVCE_SIZ_DEVICE_DESC        18
#define USB_DEIVCE_SIZ_CONFIG_DESC        (9 + NUM_INTERFACE * 9 + NUM_ENDPOINT * 7)
#define USB_DEIVCE_SIZ_STRING_LANGID      4
#define USB_DEIVCE_SIZ_STRING_VENDOR      10
#define USB_DEIVCE_SIZ_STRING_PRODUCT     28
#define USB_DEIVCE_SIZ_STRING_SERIAL      26

#define STANDARD_ENDPOINT_DESC_SIZE             0x09

#define USB_ENDPOINT_TYPE_ISOCHRONOUS           0x01
#define USB_ENDPOINT_TYPE_BULK                  0x02

/* Exported functions ------------------------------------------------------- */
extern       uint8_t Device_DeviceDescriptor[USB_DEIVCE_SIZ_DEVICE_DESC];
extern const uint8_t Device_ConfigDescriptor[USB_DEIVCE_SIZ_CONFIG_DESC];

extern const uint8_t Device_StringLangID[USB_DEIVCE_SIZ_STRING_LANGID];
extern const uint8_t Device_StringVendor[USB_DEIVCE_SIZ_STRING_VENDOR];
extern const uint8_t Device_StringProduct[USB_DEIVCE_SIZ_STRING_PRODUCT];
extern       uint8_t Device_StringSerial[USB_DEIVCE_SIZ_STRING_SERIAL];

#endif /* __USB_DESC_H */
/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
