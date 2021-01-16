#include "stm32f10x_conf.h"
#include "ModuleApi.h"
#include "Console.h"
#include "Memory.h"
#include "Flash.h"
#include "ModuleHelper.h"
#include "usb_lib.h"
#include "usb_pwr.h"
#include "usb_endp.h"

#include "stm32f10x_flash.c"

void FlashProg()
{
  DWORD  addr;
  DWORD  len;
  BYTE   loop;
  BYTE   idx;
  BYTE   err;
  BYTE   dat;
  BYTE   plen;
  WORD   rxlen;
  BYTE  *buf;
  BYTE  *txbuf;

  printf ("Waiting for data from USB ...\n");

  // Program Flash
  FLASH_Unlock ();
  FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

  addr   = 0;
  len    = 0;
  err    = 0;
  loop   = 1;

  while (loop) {
    plen = 0xFF;
    buf = UsbGetRxBuf(&rxlen);
    if (buf) {
      plen = (BYTE)rxlen;
      if (plen == EP_MAX_SIZE) { // Data packet
        // Write Flash 64 bytes
        if ((len & 0xFFF) == 0) {
          printf ("Writing flash 0x%08X - ", addr);
        }
        for (idx = 0; idx < EP_MAX_SIZE; idx += sizeof(DWORD)) {
          if ((addr >= gCommonApi->romstart) && (addr <= gCommonApi->romend)) {
            dat = 2;
          } else {
            if (FLASH_ProgramWord(addr, *(DWORD *)buf) == FLASH_COMPLETE)
              dat = 0;
            else
              dat = 1;
          }
          addr += sizeof(DWORD);
          buf  += sizeof(DWORD);
        }

        if ((len & 0xFFF) == 0) {
          if (dat==1) {
            printf ("FAIL\n");
          } else if (dat==2) {
            printf ("UNSUPPORTED\n");
          } else {
            printf ("DONE\n");
          }
        }

        err  += dat;
        len  += EP_MAX_SIZE;

      } else if (plen == CMD_PACKET_LEN) { // Command packet
        if (buf[1] == (BYTE)'A' ) { // Change address
          addr = *(DWORD *)(buf + 4);
          printf ("Setting address  0x%08X\n", addr);
        } else if (buf[1] == (BYTE)'S' ) { // Skip page
          addr += *(DWORD *)(buf + 4);
          len  += *(DWORD *)(buf + 4);
        } else if (buf[1] == (BYTE)'E' ) { // Erase block
          DWORD baddr;
          baddr = *(DWORD *)&buf[4];
          printf ("Erasing block 0x%08X - ", baddr);
          if ((baddr >= gCommonApi->romstart) && (baddr <= gCommonApi->romend)) {
            printf ("UNSUPPORTED\n");
          } else {
            if (FLASH_ErasePage(baddr) != FLASH_COMPLETE) {
              printf ("FAIL\n");
            } else {
              printf ("DONE\n");
            }
          }
        } else if (buf[1] == (BYTE)'R' ) { // Read data
            printf ("Reading block 0x%08X - ", *(DWORD *)(buf + 4));
            for (addr = 0; addr < *(DWORD *)(buf + 8); addr += EP_MAX_SIZE) {
              do {
                txbuf = UsbGetTxBuf(EP_MAX_SIZE);
              } while (!txbuf);
              if (txbuf) {
                dat = EP_MAX_SIZE;
                for (dat = 0; dat < EP_MAX_SIZE; dat++)
                  *txbuf++ = *(BYTE *)(addr+dat);
                UsbAddTxBuf (EP_MAX_SIZE);
              }
            }
            printf ("DONE\n");
        } else if (buf[1] == (BYTE)'D' ) { // Sent done
          plen = 0xF0;
        }
      }
    }

    if ((plen == 0xF0) || haschar()) {
      if (haschar()) {
        getchar();
      }
      loop = 0;
    }

    if (plen != 0xFF) {
      UsbFreeRxBuf();
    }
  }

  FLASH_Lock ();

  if (len) {
    if (err)
      printf ("Write FAIL!\n");
    else
      printf ("Write 0x%08X bytes successfully!\n", len);
  }
}

