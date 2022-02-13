#include "stm32f10x_conf.h"
#include "ModuleApi.h"
#include "Console.h"
#include "Memory.h"
#include "Flash.h"
#include "ModuleHelper.h"
#include "usb_lib.h"
#include "usb_pwr.h"
#include "usb_endp.h"


#define   CMD_HASH(x,y) (((x)<<8)| (y))

BYTE           gCmdPos;
CHAR           gCmdLine[MAX_LINE_LEN];
BYTE           gDatBuffer[256];
COMMON_API    *gCommonApi = (COMMON_API *)COMMON_API_BASE;
BYTE           gModId;
BYTE           gModFunc;

extern   void SysInit ();
extern   void FlashProg();
extern   void SendStatusResp ();

void Test()
{

}

void UsbDownload ()
{
  DWORD  addr;
  DWORD  len;
  BYTE   mode;
  BYTE   loop;
  BYTE   pktidx;
  BYTE   slot;
  BYTE   err;
  BYTE   dat;
  BYTE   plen;
  WORD   rxlen;
  BYTE  *buf;
  BYTE  *txbuf;

  mode = gCommonApi->params[1].param_l;
  printf ("Waiting for data from USB ...\n");

  if (mode == 1) {
    // Program Flash
    UnlockFlash ();
  }

  pktidx = 0;
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
        switch (mode) {
        case 0:
          // Write SRAM
          if ((len & 0xFFFF) == 0) {
            printf ("Writing SRAM 0x%08X -- ", addr);
          }
          memcpy ((void *)addr, buf, plen);
          if ((len & 0xFFFF) == 0) {
            printf ("DONE\n");
          }
          addr += plen;
          len  += plen;
          break;

        case 1:
          // Write Flash
          memcpy(gDatBuffer + (pktidx << 6), buf, plen);
          pktidx++;
          if ((pktidx & 0x03) == 0) {
            // get 256 bytes
            if ((len & 0xFFFF) == 0) {
              printf ("Writing FLASH 0x%08X -- ", addr);
            }
            dat = WriteFlash (addr >> 8, gDatBuffer);
            if ((len & 0xFFFF) == 0) {
              if (dat) {
                printf ("FAIL\n");
              } else {
                printf ("DONE\n");
              }
            }
            err  += dat;
            addr += 256;
            len  += 256;
            pktidx = 0;
          }
          break;

        default:
          break;
        }
      } else if (plen == CMD_PACKET_LEN) { // Command packet
        if (buf[1] == (BYTE)'A' ) { // Change address
          pktidx = 0;
          addr = *(DWORD *)(buf + 4);
          if (buf[3] & 0x01) { // Set module address automatically
            gCommonApi->currpage = 0xFF;
            slot = FindCurrModSlot(buf[6]);
            if ((slot == 0xFF) || (slot == (MODULE_SLOT_NUM - 1))) {
              // no space anymore, erase block
              EraseFlash(buf[6]);
            }
            slot = (slot + 1) & (MODULE_SLOT_NUM - 1);
            addr = addr + (slot * MODULE_SIZE);
            if (slot) { // Mark current block invalid
              memset ((void *)gDatBuffer, 0xFF, 256);
              *(WORD *)(gDatBuffer+2) = ~((1<<(slot+1))-1);
              WriteFlash ((addr >> 16) << 8, gDatBuffer);
            }
          }
          printf ("Setting address  0x%08X\n", addr);
        } else if (buf[1] == (BYTE)'S' ) { // Skip page
          addr += *(DWORD *)(buf + 4);
          len  += *(DWORD *)(buf + 4);
        } else if (buf[1] == (BYTE)'E' ) { // Erase block
          UnlockFlash ();
          printf ("\nErasing block 0x%0002X0000 - ", buf[6]);
          if (EraseFlash(buf[6])) {
            printf ("FAIL\n");
          } else {
            printf ("DONE\n");
          }
         } else if (buf[1] == (BYTE)'R' ) { // Read data
            printf ("Reading block 0x%08X - ", *(DWORD *)(buf + 4));
            SpiReadCmdExt(*(DWORD *)(buf + 4));
            for (addr = 0; addr < *(DWORD *)(buf + 8); addr += 64) {
              do {
                txbuf = UsbGetTxBuf(EP_MAX_SIZE);
              } while (!txbuf);
              if (txbuf) {
                dat = 64;
                if (mode == 1) { // Flash
                  for (dat = 0; dat < 64; dat++)
                    *txbuf++ = SpiWriteByte(0xFF);
                } if (mode == 0) { // SRAM or ROM
                  for (dat = 0; dat < 64; dat++)
                    *txbuf++ = *(BYTE *)(addr+dat);
                }
                UsbAddTxBuf (EP_MAX_SIZE);
              }
            }
            SpiSetCs (1);
            printf ("DONE\n");
        } else if (buf[1] == (BYTE)'H') {
          if (buf[3]) {
            SendStatusResp ();
          }
        } else if (buf[1] == (BYTE)'D' ) { // Sent done
          plen = 0xF0;
        }
      }
    } else {
      if (mode == 3) {
        // Read perf test
        txbuf = UsbGetTxBuf(EP_MAX_SIZE);
        if (txbuf) {
          memset (txbuf, 0xaa, EP_MAX_SIZE);
          UsbAddTxBuf (EP_MAX_SIZE);
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

  if (len) {
    if (err)
      printf ("Write FAIL!\n");
    else
      printf ("Write 0x%08X bytes successfully!\n", len);
  }

}

void PrintHelper(void)
{
  printf ("RB    address [length]      ;Read  Memory BYTE\n"
          "WB    address  value        ;Write Memory BYTE\n"
          "RW    address [length]      ;Read  Memory WORD\n"
          "WW    address  value        ;Write Memory WORD\n"
          "RD    address [length]      ;Read  Memory DWORD\n"
          "WD    address  value        ;Write Memory DWORD\n"
          "RF    address [length]      ;Read  SPI Flash\n"
          "ER    address [length]      ;Erase SPI Flash\n"
          "GO    address               ;Run    Module\n"
          "SM    id function           ;Select Module\n"
          "MT    arguments ...         ;Test   Module\n"
          "DL    mode                  ;Download Data from USB\n"
          "SW    [address]             ;Switch to backup flash\n"
          "RT                          ;Reset target\n"
          "FP                          ;Program IROM flash\n"
          );
}

BYTE GetUsbCommand(char *cmdline)
{
  BYTE          result;
  BYTE          len;
  BYTE         *buf;

  result = 0;

  if (!cmdline)
    cmdline = gCmdLine;

  if (mUsbRxBuf.count) {
    len = mUsbRxBuf.len[mUsbRxBuf.head];
    buf = mUsbRxBuf.buf[mUsbRxBuf.head];
    if (len == MAX_LINE_LEN) {
      // It is a command string
      memcpy (cmdline, buf, MAX_LINE_LEN);
      result = MAX_LINE_LEN;
    } else if (len == CMD_PACKET_LEN) {
      if (buf[1] == 'H') {
        if (buf[3]) {
          // Request a echo packet back
          SendStatusResp ();
        }
      }
      result = 0;
    }
    UsbFreeRxBuf();
  }

  if (result) {
    if (cmdline[0] != '!')  printf("%s\n", cmdline);
  }

  return result;
}

BYTE GetCommandLine (char *cmdline)
{
  char          ch;
  BYTE          result;

  if (!haschar())
    return 0;

  if (!cmdline)
    cmdline = gCmdLine;

  result = 0;
  ch = (int)getchar();
  if (ch == 27) {
    ch = (int)getchar();
    if (ch=='[') {
      ch = (int)getchar();
      if (ch>='A' && ch<='D') ch=0x09;
      else ch=0x80;
    }
  }

  switch (ch) {
  	case 0x09:    /* Tab */
  	  if (!gCmdPos) {
  	  	printf("%s\n", cmdline);
  	  	result = findchar(cmdline, 0) - cmdline;
  	  }
  	  break;
    case 0x08:    /* Backspace */
    case 0x7F:    /* Delete */
      if (gCmdPos > 0) {
        gCmdPos -= 1;
        putchar(0x08);  /* backspace */
        putchar(' ');
        putchar(0x08);  /* backspace */
      }
      break;
    case 0x0D:
    case 0x0A:
      putchar(0x0D);  /* CR */
      putchar(0x0A);  /* CR */
      cmdline[gCmdPos] = 0;
      result = gCmdPos + 1;
      gCmdPos = 0;
      break;
    default:
      if ((gCmdPos+1) < MAX_LINE_LEN) {
        /* only printable characters */
        if (ch > 0x1f) {
          cmdline[gCmdPos++] = (char)ch;
          putchar((char)ch);
        }
      }
      break;
  }
  return result;
}

BYTE  ExtCommand (BYTE func)
{
  BYTE           ret;
  BYTE           slot;
  BYTE           modidx;
  BYTE           cmdidx;

  // extended command
  ret = 0;
  for (modidx = 0; modidx < MODULE_COUNT; modidx++) {
    slot = FindCurrModSlot (modidx);
    if (slot == 0xFF)
      continue;
    // MOD has shell interface
    if (func == 0) {
      // Call Command
      cmdidx = FindCmdInModSlot (modidx, slot);
    } else {
      // Show help
      cmdidx = 0xEF;
    }

    if (cmdidx != 0xFF) {
      gCommonApi->cmdidx = cmdidx;
      if (CallModApi (modidx) == 1) {
        ret = 1;
      }
    }
  }
  return ret;
}

void ParseCommand (char *str)
{
  BYTE           argcnt;
  DWORD          addr;
  DWORD          len;
  DWORD          dat;
  WORD           cmd;
  BYTE           printprompt;
  BYTE           cmdstr[2];
  API_MODENTRY   modentry;
  char          *cmd_next;

  cmd_next = NULL;

  if (!str)
    str = gCmdLine;

  // ignore while space
  printprompt = 1;
  if (*str == '@' || *str == '!') {
    *str = ' ';
    if (*str == '!')
      printprompt = 0;
  }

  str = skipchar (str, ' ');

  if (*str == 0) {
    goto Quit;
  }

  if (*str == '?') {
    PrintHelper ();
    ExtCommand (1);
    goto Quit;
  }

  gCommonApi->params[0].param_l = (DWORD)str;

  if (str[1] && ((str[2] == ' ') || !str[2])) {
    cmdstr[0] = tolower(str[0]);
    cmdstr[1] = tolower(str[1]);
    cmd = CMD_HASH(cmdstr[0], cmdstr[1]);
  } else {
    cmd = 0xFFFF;
  }

  cmd_next = findchar (str, ';');
  if (*cmd_next == ';') {
    *cmd_next++ = 0;
  } else {
    cmd_next = NULL;
  }

  argcnt = 1;
  do {
    str = findchar (str, ' ');
    str = skipchar (str, ' ');
    if (*str) {
      gCommonApi->params[argcnt].param_l = xtoi(str);
      argcnt++;
    }
  } while ((*str) && (argcnt < 4));

  for (dat = argcnt; dat < 4; dat++)
      gCommonApi->params[dat].param_l = 0;

  argcnt--;
  gCommonApi->paramcnt = argcnt;

  addr = gCommonApi->params[1].param_l;
  len  = gCommonApi->params[2].param_l;
  dat  = gCommonApi->params[3].param_l;

  if (argcnt < 2) {
     len = 16;
  }

  switch(cmd) {
    case (CMD_HASH('t' , 't')): //"tt"
      Test();
      break;

    case (CMD_HASH('s' , 'm')): //"sm"
      gModId  = (BYTE)addr;
      gModFunc = (BYTE)len;
      break;

    case (CMD_HASH('m' , 't')): //"mt"
      gCommonApi->cmdidx   = gModFunc;
      if ((gModFunc & ~MODULE_COUNT_MASK) == 0xF0) {
        gCommonApi->currpage = gModId;
      }
      CallModApi(gModId);
      break;

    case (CMD_HASH('f' , 'p')): //"fp"
      FlashProg();
      break;
    case (CMD_HASH('d' , 'l')): //"dl"
      UsbDownload();
      break;
    case (CMD_HASH('r' , 'b')): //"rb"
      ReadMem(addr, len, 1);
      break;
    case (CMD_HASH('r' , 'w')): //"rw"
      ReadMem(addr, len, 2);
      break;
    case (CMD_HASH('r' , 'd')): //"rd"
      ReadMem(addr, len, 4);
      break;
    case (CMD_HASH('w' , 'b')): //"wb"
      WriteMem(addr, len, 1);
      break;
    case (CMD_HASH('w' , 'w')): //"ww"
      WriteMem(addr, len, 2);
      break;
    case (CMD_HASH('w' , 'd')): //"wd"
      WriteMem(addr, len, 4);
      break;
    case (CMD_HASH('r' , 'f')): //"rf"
      // SPI read
      if (len == 0) {
        printf ("SPI Flash ID: %06X\n", SpiReadId () & 0xFFFFFF);
      } else {
        ReadMem (addr, len, 0x11);
      }
      break;
    case (CMD_HASH('e' , 'r')): //"er"
      // Erase
      UnlockFlash ();
      if (argcnt < 2) {
        len = 1;
      } else {
        len = (len >> 16);
      }
      dat = (BYTE)(addr>>16);
      while (len--) {
        printf ("Erasing block 0x%02X0000 - ", dat);
        if (EraseFlash (dat)) {
          printf ("FAIL\n");
        } else {
          printf ("DONE\n");
        }
        dat++;
      }
      break;

    case (CMD_HASH('g' , 'o')): //"go"
      if (argcnt == 1) {
        modentry = (API_MODENTRY)addr;
        modentry ();
      }
      break;

    case (CMD_HASH('s' , 'w')): //"sw"
      __disable_irq ();
      {
        typedef void (*RST_VECT)();
        RST_VECT  ResetVector;
        if (argcnt < 1) {
          ResetVector = (RST_VECT)((*(DWORD *)(SCB->VTOR + 4)) ^ SHELL_SIZE);
        } else {
          ResetVector = (RST_VECT)(*(DWORD *)(addr + 4));
        }
        ResetVector();
      }
      break;

    case (CMD_HASH('r' , 't')): //"rt"
      if (argcnt > 0) {
        *(UINT8 *)USER_REG_BASE = (UINT8)addr;
      }
      NVIC_SystemReset ();
      break;

    default:
      cmd = 0xFFFF;
      break;
  }

  if (cmd == 0xFFFF) {
    // extended command
    if (!ExtCommand (0)) {
      printf ("Unknown command!\n", len);
      goto Quit;
    }
  }

Quit:
  if (printprompt) printf ("%s", cmd_next ? "\n" : "\n>");

  if (cmd_next) {
    ParseCommand (cmd_next);
  }

  return;
}

void PrintBanner ()
{
  // Main loop
  printf ("\n\n"
          "*******************************\n"
          "*  Welcome to STM32F103("
#if RECOVERY
          "R"
#else
          "P"
#endif
          PACKAGE_PINS ")  *\n"
          "*     Strong Shell  V1.12     *\n"
          "*         Micro  Blue         *\n"
          "*******************************\n"
          "\n\n>");
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
#define SHOW_BANNER_DELAY  0x2

void SwitchPart ()
{
  typedef void (*RST_VECT)();
  RST_VECT  ResetVector;
  UINT8     Magic;

  Magic = *(UINT8 *)USER_REG_BASE;
  *(UINT8 *)USER_REG_BASE = 0x00;
  ResetVector = (RST_VECT)0;
  switch(Magic) {
  case 0xAA:
    ResetVector = (RST_VECT)((*(DWORD *)(SCB->VTOR + 4)) ^ SHELL_SIZE);
    break;
  case 0xAB:
    ResetVector = (RST_VECT)((*(DWORD *)(SCB->VTOR + 4)));
    break;
  case 0xAC:
    ResetVector = (RST_VECT)(*(DWORD *)(USER_REG_BASE + 0x04));
    break;
  case 0xAD:
    ResetVector = (RST_VECT)(*(DWORD *)(USER_REG_BASE + 0x04));
    ResetVector = (RST_VECT)(*(DWORD *)((UINT8 *)ResetVector + 0x04));
    break;
  default:
    break;
  }

  if (ResetVector) {
    ResetVector();
  }

}

#define  MODULE_ROM_BASE  0x08018000

int main(void)
{
  BYTE           bShowBanner = 0;
  BYTE           OldCon;
  API_MODENTRY   ModEntry;

  __disable_irq ();

  // Check boot from which partition
  SwitchPart ();

  SysInit();

  // Turn on LEDs
  LED_A_ON;
  LED_B_OFF;

  PrintBanner ();
  __enable_irq ();

  // Check boot mode
  if (ENABLE_USER_APP && (GPIOA->IDR & GPIO_Pin_14)) {
    // Run user command
    ParseCommand("USER");
    // Run hardcoded base
    if (*(BYTE *)(MODULE_ROM_BASE) == 0xA5) {
      memcpy ((void *)MODULE_BASE, (void *)MODULE_ROM_BASE, KB(15));
      gModEntry ();
    } else if (*(BYTE *)(MODULE_ROM_BASE) == 0x5A) {
      ModEntry = (API_MODENTRY) (MODULE_ROM_BASE + sizeof(MODULE_HEADER) + 1);
      ModEntry ();
    }
  }

  while (1)
  {
    //if (bDeviceState == CONFIGURED) {
    if (IsEpTxReady(1)) {
      if (!bShowBanner) {
        bShowBanner = 1;
        SetCon (USB_CON);
        PrintBanner();
        // SetCon (USB_CON | SERIAL_CON);
      }
    }

    if (GetCommandLine(NULL)) {
      ParseCommand(NULL);
    }

    // It is a command string
    if (GetUsbCommand(NULL)) {
      // Disable USB console ouput to prevent EP full
      if (gCmdLine[0] == '!') {
        OldCon = SetCon (0);
        ParseCommand(NULL);
        SetCon (OldCon);
      } else {
        printf ("\n");
        ParseCommand(NULL);
      }
    }
  }

  return 0;
}

//{void putsc (char ch); putsc('$');}