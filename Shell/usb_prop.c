/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
* File Name          : usb_prop.c
* Author             : MCD Application Team
* Version            : V3.3.0
* Date               : 21-March-2011
* Description        : All processing related to Virtual Com Port Demo
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "Common.h"
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usb_endp.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t Request = 0;

/* -------------------------------------------------------------------------- */
/*  Structures initializations */
/* -------------------------------------------------------------------------- */

DEVICE Device_Table =
{
    EP_NUM,
    1
};

DEVICE_PROP Device_Property =
  {
    Device_Init,
    Device_Reset,
    Device_Status_In,
    Device_Status_Out,
    Device_Data_Setup,
    Device_NoData_Setup,
    Device_Get_Interface_Setting,
    Device_GetDeviceDescriptor,
    Device_GetConfigDescriptor,
    Device_GetStringDescriptor,
    0,
    EP_MAX_SIZE /*MAX PACKET SIZE*/
  };

USER_STANDARD_REQUESTS User_Standard_Requests =
  {
    Device_GetConfiguration,
    Device_SetConfiguration,
    Device_GetInterface,
    Device_SetInterface,
    Device_GetStatus,
    Device_ClearFeature,
    Device_SetEndPointFeature,
    Device_SetDeviceFeature,
    Device_SetDeviceAddress
  };

ONE_DESCRIPTOR Device_Descriptor =
  {
    (uint8_t*)Device_DeviceDescriptor,
    USB_DEIVCE_SIZ_DEVICE_DESC
  };

ONE_DESCRIPTOR Config_Descriptor =
  {
    (uint8_t*)Device_ConfigDescriptor,
    USB_DEIVCE_SIZ_CONFIG_DESC
  };

ONE_DESCRIPTOR String_Descriptor[4] =
  {
    {(uint8_t*)Device_StringLangID, USB_DEIVCE_SIZ_STRING_LANGID},
    {(uint8_t*)Device_StringVendor, USB_DEIVCE_SIZ_STRING_VENDOR},
    {(uint8_t*)Device_StringProduct, USB_DEIVCE_SIZ_STRING_PRODUCT},
    {(uint8_t*)Device_StringSerial, USB_DEIVCE_SIZ_STRING_SERIAL}
  };

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Extern function prototypes ------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : Device_init.
* Description    : Virtual COM Port Mouse init routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Device_Init(void)
{ 

  /* Update the serial number string descriptor with the data from the unique
  ID*/
  pInformation->Current_Configuration = 0;

  /* Connect the device */
  PowerOn();

  /* Perform basic device initialization operations */
  USB_SIL_Init();
  
  bDeviceState = UNCONNECTED;
}

/*******************************************************************************
* Function Name  : Device_Reset
* Description    : Virtual_Com_Port Mouse reset routine
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Device_Reset(void)
{
  // Set Virtual_Com_Port DEVICE as not configured
  pInformation->Current_Configuration = 0;

  // Current Feature initialization
  pInformation->Current_Feature = Device_ConfigDescriptor[7];

  // Set Virtual_Com_Port DEVICE with the default Interfac
  pInformation->Current_Interface = 0;

  SetBTABLE(BTABLE_ADDRESS);

  // Initialize Endpoint 0
  SetEPType(ENDP0, EP_CONTROL);
  SetEPTxStatus(ENDP0, EP_TX_STALL);
  SetEPRxAddr(ENDP0, ENDP0_RXADDR);
  SetEPTxAddr(ENDP0, ENDP0_TXADDR);
  Clear_Status_Out(ENDP0);
  SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
  //SetEPRxValid(ENDP0);

  // Initialize Endpoint 1
  SetEPType(ENDP1, EP_BULK);
  SetEPTxStatus(ENDP1, EP_TX_NAK);
  SetEPRxAddr(ENDP1, ENDP1_RXADDR);
  SetEPTxAddr(ENDP1, ENDP1_TXADDR);
  Clear_Status_Out(ENDP1);
  SetEPRxCount(ENDP1, Device_Property.MaxPacketSize);
  SetEPRxValid(ENDP1);

  // Initialize Endpoint 2  OUT
  // Double buffering
  SetEPType(ENDP2, EP_BULK);
#if RX_DOUBLE_BUFFER  
  SetEPDoubleBuff(ENDP2);         
  SetEPDblBuffAddr(ENDP2, ENDP2_RXADDR0, ENDP2_RXADDR1);
  SetEPDblBuffCount(ENDP2, EP_DBUF_OUT, Device_Property.MaxPacketSize);
  ClearDTOG_RX(ENDP2);
  ClearDTOG_TX(ENDP2);
  ToggleDTOG_TX(ENDP2);  
#else  
  SetEPRxAddr(ENDP2, ENDP2_RXADDR0);
  SetEPRxCount(ENDP2, EP_MAX_SIZE);  
#endif
  SetEPRxStatus(ENDP2, EP_RX_VALID);
  SetEPTxStatus(ENDP2, EP_TX_DIS);

  // Initialize Endpoint 3  IN
  // Double buffering
  SetEPType(ENDP3, EP_BULK);
#if TX_DOUBLE_BUFFER  
  SetEPDoubleBuff(ENDP3); 
  SetEPDblBuffAddr(ENDP3, ENDP3_TXADDR0, ENDP3_TXADDR1);
  SetEPDblBuffCount(ENDP3, EP_DBUF_IN, Device_Property.MaxPacketSize);
  ClearDTOG_RX(ENDP3);
  ClearDTOG_TX(ENDP3);
  ToggleDTOG_RX(ENDP3);  
#else  
  SetEPTxAddr(ENDP3, ENDP3_TXADDR0);
  SetEPTxCount(ENDP3, EP_MAX_SIZE);  
#endif  
  SetEPRxStatus(ENDP3, EP_RX_DIS);
  SetEPTxStatus(ENDP3, EP_TX_NAK);

  // Set this device to response on default address  
  SetEPRxValid(ENDP0);
  SetDeviceAddress(0);  

  bDeviceState = ATTACHED;
}

/*******************************************************************************
* Function Name  : Device_SetConfiguration.
* Description    : Update the device state to configured.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Device_SetConfiguration(void)
{
  DEVICE_INFO *pInfo = &Device_Info;

  if (pInfo->Current_Configuration != 0)
  {
    /* Device configured */
    bDeviceState = CONFIGURED; 

    /* Prepare dummy data for ENDP0 */
    WriteEpFromBuffer(ENDP1, NULL, 0);   
  }
}

/*******************************************************************************
* Function Name  : Device_SetConfiguration.
* Description    : Update the device state to addressed.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Device_SetDeviceAddress (void)
{
  bDeviceState = ADDRESSED;
}

/*******************************************************************************
* Function Name  : Device_Status_In.
* Description    : Virtual COM Port Status In Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Device_Status_In(void)
{
}

/*******************************************************************************
* Function Name  : Device_Status_Out
* Description    : Virtual COM Port Status OUT Routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Device_Status_Out(void)
{
}

/*******************************************************************************
* Function Name  : Device_Data_Setup
* Description    : handle the data class specific requests
* Input          : Request Nb.
* Output         : None.
* Return         : USB_UNSUPPORT or USB_SUCCESS.
*******************************************************************************/
RESULT Device_Data_Setup(uint8_t RequestNo)
{
  return USB_UNSUPPORT;  
}

/*******************************************************************************
* Function Name  : Device_NoData_Setup.
* Description    : handle the no data class specific requests.
* Input          : Request Nb.
* Output         : None.
* Return         : USB_UNSUPPORT or USB_SUCCESS.
*******************************************************************************/
RESULT Device_NoData_Setup(uint8_t RequestNo)
{
  return USB_UNSUPPORT;
}

/*******************************************************************************
* Function Name  : Device_GetDeviceDescriptor.
* Description    : Gets the device descriptor.
* Input          : Length.
* Output         : None.
* Return         : The address of the device descriptor.
*******************************************************************************/
uint8_t *Device_GetDeviceDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Device_Descriptor);
}

/*******************************************************************************
* Function Name  : Device_GetConfigDescriptor.
* Description    : get the configuration descriptor.
* Input          : Length.
* Output         : None.
* Return         : The address of the configuration descriptor.
*******************************************************************************/
uint8_t *Device_GetConfigDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Config_Descriptor);
}

/*******************************************************************************
* Function Name  : Device_GetStringDescriptor
* Description    : Gets the string descriptors according to the needed index
* Input          : Length.
* Output         : None.
* Return         : The address of the string descriptors.
*******************************************************************************/
uint8_t *Device_GetStringDescriptor(uint16_t Length)
{
  uint8_t wValue0 = pInformation->USBwValue0;
  if (wValue0 > 4)
  {
    return NULL;
  }
  else
  {
    return Standard_GetDescriptorData(Length, &String_Descriptor[wValue0]);
  }
}

/*******************************************************************************
* Function Name  : Device_Get_Interface_Setting.
* Description    : test the interface and the alternate setting according to the
*                  supported one.
* Input1         : uint8_t: Interface : interface number.
* Input2         : uint8_t: AlternateSetting : Alternate Setting number.
* Output         : None.
* Return         : The address of the string descriptors.
*******************************************************************************/
RESULT Device_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting)
{  
  if (AlternateSetting > 0)
  {
    return USB_UNSUPPORT;
  }
  else if (Interface > 1)
  {
    return USB_UNSUPPORT;
  }
  return USB_SUCCESS;
}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

