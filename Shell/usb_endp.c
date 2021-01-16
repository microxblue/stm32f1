/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
* File Name          : usb_endp.c
* Author             : MCD Application Team
* Version            : V3.3.0
* Date               : 21-March-2011
* Description        : Endpoint routines
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "usb_endp.h"
#include "usb_desc.h"
#include "usb_mem.h"
#include "hw_config.h"
#include "usb_istr.h"
#include "usb_pwr.h"


USB_RX_BUF  mUsbRxBuf;
USB_TX_BUF  mUsbTxBuf;

BYTE    mEpRxBufSts;
BYTE    mEpTxBufSts;
BYTE    mConsoleConnected;

void RESET_Callback(void)
{
  uint8_t  i;

  mEpRxBufSts = 0;
  mConsoleConnected  = 0;  

  for (i = 0; i < MAX_RX_PKT_NUM; i++) {
    mUsbRxBuf.len[i] = 0xFF;
  }
  mUsbRxBuf.tail   = 0;
  mUsbRxBuf.head   = 0;
  mUsbRxBuf.count  = 0;  
  mUsbRxBuf.tofree = 0;

  for (i = 0; i < MAX_TX_PKT_NUM; i++) {
    mUsbTxBuf.len[i] = 0xFF;
  }
  mUsbTxBuf.tail   = 0;
  mUsbTxBuf.head   = 0;
  mUsbTxBuf.count  = 0;    
}

BYTE IsConsoleConnected ()
{
  return mConsoleConnected;
}

BYTE IsEpTxReady (BYTE Ep)
{
  return mEpTxBufSts & (1 << Ep);
}

WORD WriteEpFromBuffer (BYTE Ep, BYTE *Buf, WORD Len)
{  
  if (Ep == 1) {
    if (Len > EP_MAX_SIZE) Len = EP_MAX_SIZE; 
    mEpTxBufSts &= ~(1<<1);
    UserToPMABufferCopy(Buf, GetEPTxAddr(ENDP1), Len);
    SetEPTxCount(ENDP1, Len);
    SetEPTxValid(ENDP1);        //enable endpoint for transmission         
  } else {
    Len = 0;
  }
  return Len;
}

BYTE IsEpRxReady (BYTE Ep)
{
  return mEpRxBufSts & (1 << Ep);
}

WORD ReadEpToBuffer (BYTE Ep, BYTE *Buf)
{  
  WORD Data_Len;       /* data length*/    
  Data_Len = 0;
  if (Ep == 1) {
    Data_Len = GetEPRxCount(ENDP1);
    PMAToUserBufferCopy(Buf, ENDP1_RXADDR, Data_Len);
    SetEPRxValid(ENDP1);
  }
  mEpRxBufSts &= ~(1 << 1);
  return Data_Len;
}

void EP1_OUT_Callback (void)
{  
  if (!(mEpRxBufSts & (1 << 1))) {  
    // Mark EP1 Rx Buffer ready    
    if (GetEPRxCount(ENDP1) == 0) {    
      SetEPRxValid(ENDP1);
    } else {
      mEpRxBufSts |= (1 << 1);
    }
  } 
}

void EP1_IN_Callback (void)
{
  if (!mConsoleConnected) {
    mConsoleConnected = 1;
  }
  if (!(mEpTxBufSts & (1 << 1))) {  
    mEpTxBufSts |= (1<<1);
  }
}

uint8_t *UsbGetRxBuf(WORD *len)
{
  uint8_t  *buf;
  
  if (mUsbRxBuf.count) {    
    *len = mUsbRxBuf.len[mUsbRxBuf.head];    
    buf  = mUsbRxBuf.buf[mUsbRxBuf.head];
  } else {
    *len = 0;
    buf  = NULL;
  }

  return buf;
}  

void UsbFreeRxBuf()
{
  EnterCriticalRegion();            
  mUsbRxBuf.len[mUsbRxBuf.head] = 0xFF;
  if (++mUsbRxBuf.head >= MAX_RX_PKT_NUM) {
    mUsbRxBuf.head = 0;
  }       
  mUsbRxBuf.count--;
  if (mUsbRxBuf.tofree) {
    mUsbRxBuf.tofree--;
    FreeUserBuffer(ENDP2, EP_DBUF_OUT);
  }
  ExitCriticalRegion();    
}

uint8_t *UsbGetTxBuf(WORD len)
{
  BYTE  pktnum;
  pktnum = len ? (len + EP_MAX_SIZE - 1) >> 6 : 1;  
  if (pktnum <= MAX_TX_PKT_NUM - mUsbTxBuf.count) {
    // fill data to buffer
    return mUsbTxBuf.buf[mUsbTxBuf.tail];
  } else {
    return NULL;
  }  
}

void UsbAddTxBuf(WORD len)
{
  BYTE  pktnum;
  BYTE  pktlen;

  pktnum = 0;
  do {
    pktlen = (len > EP_MAX_SIZE) ? EP_MAX_SIZE : len;
    len   -= pktlen;      
    mUsbTxBuf.len[mUsbTxBuf.tail] = pktlen;
    if (++mUsbTxBuf.tail >= MAX_TX_PKT_NUM) {
      mUsbTxBuf.tail = 0;
    }      
    pktnum++;
  } while (len);

  EnterCriticalRegion(); 
  mUsbTxBuf.count += pktnum;
  if (GetEPTxStatus (ENDP3) == EP_TX_NAK) {    
    EP3_IN_Callback ();
  }        
  ExitCriticalRegion(); 
}

void EP2_OUT_Callback(void)
{
  uint16_t  buf_len;
  uint16_t  buf_addr;
  uint8_t   tail;
  
  if (mUsbRxBuf.count > MAX_RX_PKT_NUM) {
    //tprintf("RX Buffer Overflow!\n");
    return;
  }

  tail   = mUsbRxBuf.tail;  
  if (GetENDPOINT(ENDP2) & EP_DTOG_TX) {    
    buf_len  = GetEPDblBuf0Count(ENDP2);
    buf_addr = ENDP2_RXADDR0;    
  } else {    
    buf_len  = GetEPDblBuf1Count(ENDP2);
    buf_addr = ENDP2_RXADDR1;    
  }
  PMAToUserBufferCopy (mUsbRxBuf.buf[tail], buf_addr, buf_len);    
  if (MAX_RX_PKT_NUM - mUsbRxBuf.count >= 2) {
    FreeUserBuffer(ENDP2, EP_DBUF_OUT);   
  } else {
    mUsbRxBuf.tofree++;
  }
  mUsbRxBuf.len[tail] = buf_len;   
  if (++tail >= MAX_RX_PKT_NUM) {
    tail = 0;
  }
  mUsbRxBuf.tail = tail;
  mUsbRxBuf.count++;  
}

void EP3_IN_Callback(void)
{
  uint8_t  *buf;
  uint8_t   head;
  uint16_t  len;

  if (mUsbTxBuf.count) {
    if (GetEPTxStatus (ENDP3) == EP_TX_NAK) {  
      // Single buffer
      head = mUsbTxBuf.head;
      buf = mUsbTxBuf.buf[head];
      len = mUsbTxBuf.len[head];
#if TX_DOUBLE_BUFFER
      FreeUserBuffer(ENDP3, EP_DBUF_IN);
      if (GetENDPOINT(ENDP3) & EP_DTOG_RX) {      
        UserToPMABufferCopy(buf, ENDP3_TXADDR0, len);
        SetEPDblBuf0Count(ENDP3, EP_DBUF_IN, len);
      } else {
        UserToPMABufferCopy(buf, ENDP3_TXADDR1, len);
        SetEPDblBuf1Count(ENDP3, EP_DBUF_IN, len);
      }
#else
      UserToPMABufferCopy(buf, GetEPTxAddr(ENDP3), len);
      SetEPTxCount(ENDP3, len);      
#endif
      SetEPTxValid(ENDP3);      
      mUsbTxBuf.len[head] = 0xFF; 
      if (++head >= MAX_TX_PKT_NUM) {
        head = 0;
      }
      mUsbTxBuf.head = head;
      mUsbTxBuf.count--; 
    }
  }
}

#if 0
void PrintTxBufSts()
{
  int i;
  
  tprintf ("head : %d\n", mUsbTxBuf.head);
  tprintf ("tail : %d\n", mUsbTxBuf.tail);
  tprintf ("count: %d\n", mUsbTxBuf.count);  
  for (i=0; i < MAX_TX_PKT_NUM; i++) {
    tprintf ("%d:  Len=0x%02X\n", i, mUsbTxBuf.len[i]);
  }
  tprintf("\n");
}

void PrintRxBufSts()
{
  int i;
  
  tprintf ("head : %d\n", mUsbRxBuf.head);
  tprintf ("tail : %d\n", mUsbRxBuf.tail);
  tprintf ("count: %d\n", mUsbRxBuf.count);
  tprintf ("free : %d\n", mUsbRxBuf.tofree);
  for (i=0; i < MAX_RX_PKT_NUM; i++) {
    tprintf ("%d:  Len=0x%02X\n", i, mUsbRxBuf.len[i]);
  }
  tprintf("\n");
}
#endif

