#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "Common.h"

#define  SERIAL_CONSOLE  0

#ifndef  USB_CONSOLE
#define  USB_CONSOLE     1
#endif

#if (!(SERIAL_CONSOLE | USB_CONSOLE))
#error "At least one default console (SERIAL or USB) needs to be defined !"
#endif

#define  SERIAL_CON        1
#define  USB_CON           2

#define  USB_SEND_RETRY    16
#define  MAX_IN_LEN        64

#define  MAX_LINE_LEN      32

#if (!USB_CONSOLE)
#define  usbcon_connected()  (1)
#define  usbcon_commit()
#define  USB_Istr()
#define  USB_Init()
#else
BYTE     usbcon_connected ();
void     usbcon_commit () ;
#endif

BYTE GetCon ();

BYTE SetCon (BYTE value);

char getsc   ();
char getchar ();

void putsc   (char c);
void putchar (char c);

char hassc   ();
char haschar ();


#endif