#include "ModuleApi.h"
#include "Flash.h"
#include "usb_endp.h"

API_MODENTRY   gModEntry = (API_MODENTRY)MODULE_ENTRY;
BYTE           gModInSpi = 1;

void ModInit ()
{
  extern DWORD __isr_vector;
  extern DWORD __etext;

  memset(gCommonApi, 0x00, sizeof(COMMON_API));

  gCommonApi->boardid      = BOARD_ID;
  gCommonApi->currpage     = 0xFF;
  gCommonApi->romstart     = ((DWORD)(&__isr_vector) + 0x3FF) & 0xFFFFFC00;
  gCommonApi->romend       = ((DWORD)(&__etext) + 0x3FF) & 0xFFFFFC00;

  gCommonApi->HasChar      = HasChar;
  gCommonApi->GetChar      = GetChar;
  gCommonApi->PutChar      = PutChar;
  gCommonApi->Printf       = Printf;

  gCommonApi->UsbGetRxBuf  = UsbGetRxBuf;
  gCommonApi->UsbFreeRxBuf = UsbFreeRxBuf;
  gCommonApi->UsbGetTxBuf  = UsbGetTxBuf;
  gCommonApi->UsbAddTxBuf  = UsbAddTxBuf;

  gCommonApi->GpioInit     = (API_GPIO_INIT)GPIO_Init;
  gCommonApi->SpiWrite     = ExtSpiWriteByte;
  gCommonApi->NvicInit     = (API_NVIC_INIT)NVIC_Init;
  gCommonApi->UartInit     = (API_UART_INIT)USART_Init;

  gCommonApi->SetCon       = (API_SET_CON)SetCon;
}

BYTE FindCmdInModSlot (BYTE blkidx, BYTE slot)
{
  BYTE  ret;
  BYTE  len;
  BYTE  idx;
  DWORD addr;
  char  command[CMD_NUM][CMD_LEN];
  char *ptr;

  ret  = 0xFF;
  addr = (DWORD)(blkidx + MODULE_BLK_BASE) << 16;
  addr+= (MODULE_SIZE * slot + STRUCT_OFF(MODULE_HEADER, command));
  len  = CMD_NUM * CMD_LEN;
  ptr  = (char *)command;
  SpiReadCmdExt (addr);
  while (len--)  *ptr++ =  SpiWriteByte(0xFF);
  SpiSetCs (1);

  ptr = (char *)(gCommonApi->params[0].param_l);
  ptr = skipchar(ptr, ' ');
  if (!ptr[CMD_LEN] || ptr[CMD_LEN] == ' ') {
    for (idx = 0; idx < CMD_NUM; idx++) {
      if (!strncmpi(ptr, command[idx], CMD_LEN)) {
        ret = idx;
        break;
      }
    }
  }

  return ret;
}

BYTE FindCurrModSlot (BYTE blkidx)
{
  WORD  mask;
  WORD  marker;
  DWORD addr;
  BYTE  seg;

  blkidx &= 0x0F;
  addr = (DWORD)(blkidx + MODULE_BLK_BASE) << 16;
  SpiReadCmdExt (addr);
  ((BYTE *)&marker)[0]  = SpiWriteByte(0xFF);
  ((BYTE *)&marker)[1]  = SpiWriteByte(0xFF);
  ((BYTE *)&mask)[0]    = SpiWriteByte(0xFF);
  ((BYTE *)&mask)[1]    = SpiWriteByte(0xFF);
  SpiSetCs(1);

  if (marker == 0x5AA5) {
    mask &= MODULE_SLOT_MASK;
    // no more empty slot
    if (mask == 0)
      return MODULE_SLOT_NUM - 1;

    seg = 0;
    while (mask) {
      if (mask & 1) {
        break;
      }
      mask >>= 1;
      seg++;
    }
    if (seg == 0)
      return 0xFF;
    else
      return seg - 1;
  } else {
    // Invalid module block
    return 0xFF;
  }
}

BYTE LoadModulePage (BYTE blkidx)
{
  BYTE  ret;
  BYTE  seg;
  WORD  len;
  BYTE *ptr;
  DWORD addr;

  // Read flash
  blkidx &= 0x0F;
  seg = FindCurrModSlot (blkidx);
  if (seg == 0xFF) {
    ret = (BYTE)0xFF;
  } else {
    // Read again
    addr = (DWORD)(blkidx + MODULE_BLK_BASE) << 16;
    addr+= (MODULE_SIZE * seg);
    SpiReadCmdExt (addr);
    ptr = (BYTE *)MODULE_BASE;
    len = MODULE_SIZE;
    while (len--) *ptr++ = SpiWriteByte(0xFF);
    // Update active page
    gCommonApi->currpage = blkidx;
    ret  = 0;
  }

  // Release CS
  SpiSetCs (1);

  return ret;
}

BYTE CallModApi (BYTE blkidx)
{
  if (blkidx & 0x80) {
    gModEntry = (API_MODENTRY)MODULE_ENTRY_ROM;
  } else {
    gModEntry = (API_MODENTRY)MODULE_ENTRY;
    if (gCommonApi->currpage != blkidx)
    {
      if (LoadModulePage (blkidx) & 0x80) {
        return 0;
      }
    }
  }

  return gModEntry ();
}