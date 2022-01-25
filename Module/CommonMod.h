#ifndef _COMMON_MOD_H_
#define _COMMON_MOD_H_

#define __MODULE__

#include "..\Shell\ModuleApi.h"

#define  ZeroBss() { \
    extern unsigned int __bss_start__, __bss_end__; \
    int *p = (int *)&__bss_start__; \
    while (p < (int *)&__bss_end__) *p++ = 0; \
  }

#define  printf(fmt, args...)   gCommonApi->Printf(fmt, ##args)
#define  getchar                gCommonApi->GetChar
#define  haschar                gCommonApi->HasChar
#define  putchar                gCommonApi->PutChar
#define  spiwrite               gCommonApi->SpiWrite
#define  gpioinit               gCommonApi->GpioInit
#define  nvicinit               gCommonApi->NvicInit
#define  setcon                 gCommonApi->SetCon

#define  UsbGetRxBuf            gCommonApi->UsbGetRxBuf
#define  UsbFreeRxBuf           gCommonApi->UsbFreeRxBuf
#define  UsbGetTxBuf            gCommonApi->UsbGetTxBuf
#define  UsbAddTxBuf            gCommonApi->UsbAddTxBuf

#endif