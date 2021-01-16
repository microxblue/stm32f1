#ifndef __USB_ENDP_H
#define __USB_ENDP_H

#include "Common.h"
#include "usb_lib.h"
#include "usb_desc.h"

#define RX_DOUBLE_BUFFER   1
#define TX_DOUBLE_BUFFER   0

#define MAX_RX_PKT_NUM  8
#define MAX_TX_PKT_NUM  4

#define EnterCriticalRegion()   __disable_irq ();    
#define ExitCriticalRegion()    __enable_irq ();  

typedef struct {
  uint8_t  buf[MAX_RX_PKT_NUM][EP_MAX_SIZE];
  uint8_t  len[MAX_RX_PKT_NUM];
  uint8_t  head;
  uint8_t  tail;
  uint8_t  count;
  uint8_t  tofree;
} USB_RX_BUF;

typedef struct {
  uint8_t  buf[MAX_TX_PKT_NUM][EP_MAX_SIZE];
  uint8_t  len[MAX_TX_PKT_NUM];
  uint8_t  head;
  uint8_t  tail;
  uint8_t  count;  
} USB_TX_BUF;

extern  USB_RX_BUF  mUsbRxBuf;
extern  USB_TX_BUF  mUsbTxBuf;

uint8_t *UsbGetRxBuf(WORD *len);
void     UsbFreeRxBuf();
uint8_t *UsbGetTxBuf(WORD  len);
void     UsbAddTxBuf(WORD  len);

BYTE IsConsoleConnected ();
BYTE IsEpRxReady        (BYTE Ep);
BYTE IsEpTxReady        (BYTE Ep);
WORD ReadEpToBuffer     (BYTE Ep, BYTE *Buf);
WORD WriteEpFromBuffer  (BYTE Ep, BYTE *Buf, WORD Len);

void UsbRxAck ();

#endif
