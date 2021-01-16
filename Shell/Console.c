#include "Console.h"
#include "stm32f10x_conf.h"
#include "usb_endp.h"

BYTE  gConInFlag = SERIAL_CONSOLE;

#if SERIAL_CONSOLE
void putsc (char ch)
{
  while (!(USART1->SR & USART_FLAG_TXE));
  USART1->DR = ch;
}

char hassc ()
{
  return USART1->SR & USART_FLAG_RXNE;
}

char getsc ()
{
  while (!HasChar());
  return USART1->DR;
}
#endif

#if USB_CONSOLE

// REQ will be set/clear by apps only
volatile BYTE gUsbConInPos    = 0;
volatile BYTE gUsbConInCnt    = 0;
volatile BYTE gUsbConOutPos   = 0;
volatile BYTE gUsbConOutRetry = 0;

BYTE    EP1_OUT_PACKET[64];
BYTE    EP1_IN_PACKET[64];

static
void putuc (char c)
{
   if (gUsbConOutPos >= MAX_IN_LEN) {
     // If USB is not connected, drop it
     if (!(_GetFNR() & FNR_LCK)) {
       return;
     }
     while (gUsbConOutPos);
   }

   // Disable timer interrupt
   __disable_irq();
   EP1_IN_PACKET[gUsbConOutPos++] = c;
   __enable_irq();
}

static
BYTE hasuc ()
{
  if (IsEpRxReady(1) || gUsbConInPos) {
    return 1;
  } else {
    return 0;
  }
}

static
char getuc ()
{
  char ch;

  while (!hasuc());

  if (!gUsbConInPos) {
    __disable_irq();
    gUsbConInCnt = ReadEpToBuffer (1, (BYTE*)EP1_OUT_PACKET);
    __enable_irq();
  }

  ch = EP1_OUT_PACKET[gUsbConInPos++];
  if (gUsbConInPos >= gUsbConInCnt) {
    gUsbConInPos = 0;
  }

  return ch;
}

BYTE usbcon_connected ()
{
  return IsConsoleConnected();
}

void usbcon_commit ()
{
  WORD  Written;

  // Send Data
  if (gUsbConOutPos) {

    if (IsEpTxReady(1)) {
      __disable_irq();
      Written = WriteEpFromBuffer (1, (BYTE *)EP1_IN_PACKET, gUsbConOutPos);
      __enable_irq();
    } else {
      Written = 0;
    }

    if (Written < gUsbConOutPos) {
      // Not ready for send yet
      if (gUsbConOutRetry >= USB_SEND_RETRY) {
        gUsbConOutPos = 0;
        // We may have lost connection
      } else {
        gUsbConOutRetry++;
      }
      // gUsbConOutRetry = 1;
    } else {
      gUsbConOutRetry = 0;
      gUsbConOutPos   = 0;
    }
  }
}

#endif


char HasChar ()
{
#if SERIAL_CONSOLE
  if ((gConInFlag & SERIAL_CON) && hassc()) {
    return SERIAL_CON;
  }
#endif
#if USB_CONSOLE
  if ((gConInFlag & USB_CON) && hasuc()) {
    return USB_CON;
  }
#endif
  return 0;
}

void PutChar (char c)
{
#if SERIAL_CONSOLE
  if (gConInFlag & SERIAL_CON)
    putsc (c);
#endif
#if USB_CONSOLE
  if (gConInFlag & USB_CON)
    putuc (c);
#endif
}

char GetChar ()
{
  BYTE src;
  do {
    src = haschar();
  } while (!src);

#if SERIAL_CONSOLE
  if (src == SERIAL_CON) {
    return getsc();
  }
#endif

#if USB_CONSOLE
  if (src == USB_CON) {
    return getuc();
  }
#endif

  return 0;
}

BYTE SetCon (BYTE value)
{
   BYTE  old;
   old = gConInFlag;
   gConInFlag = value;
   return old;
}

void Puts(const char *s)
{
  while (*s) PutChar(*s);
}
