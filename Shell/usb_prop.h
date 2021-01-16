/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
* File Name          : usb_prop.h
* Author             : MCD Application Team
* Version            : V3.3.0
* Date               : 21-March-2011
* Description        : All processing related to Virtual COM Port Demo (Endpoint 0)
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __usb_prop_H
#define __usb_prop_H

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported define -----------------------------------------------------------*/

#define Device_GetConfiguration          NOP_Process
//#define Device_SetConfiguration          NOP_Process
#define Device_GetInterface              NOP_Process
#define Device_SetInterface              NOP_Process
#define Device_GetStatus                 NOP_Process
#define Device_ClearFeature              NOP_Process
#define Device_SetEndPointFeature        NOP_Process
#define Device_SetDeviceFeature          NOP_Process
//#define Device_SetDeviceAddress          NOP_Process

#define SEND_ENCAPSULATED_COMMAND   0x00
#define GET_ENCAPSULATED_RESPONSE   0x01
#define SET_COMM_FEATURE            0x02
#define GET_COMM_FEATURE            0x03
#define CLEAR_COMM_FEATURE          0x04

/* Exported functions ------------------------------------------------------- */
void     Device_Init(void);
void     Device_Reset(void);
void     Device_SetConfiguration(void);
void     Device_SetDeviceAddress (void);
void     Device_Status_In (void);
void     Device_Status_Out (void);
RESULT   Device_Data_Setup(uint8_t);
RESULT   Device_NoData_Setup(uint8_t);
RESULT   Device_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting);
uint8_t *Device_GetDeviceDescriptor(uint16_t );
uint8_t *Device_GetConfigDescriptor(uint16_t);
uint8_t *Device_GetStringDescriptor(uint16_t);

#endif /* __usb_prop_H */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

