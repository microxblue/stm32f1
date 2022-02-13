#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "CommonMod.h"
#include "ModComLib.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_spi.h"

#define SYS_CLK  72000000

#define  SERVO_OPEN_VAL     1700
#define  SERVO_CLOSE_VAL    2240

#define TRUE   1
#define FALSE  0

// 0: bit[7:0] signature low
// 1: bit[7:0] signature high
// 2: bit[3:0] module ID (0-F)
//    bit[4]   command interface  0:has  1:none
// 3: bit[7:0] board support bit map

__attribute__((section (".header")))
const MODULE_HEADER gModuleHeader = {
  0x5AA5,     // Signature Marker
  0xFFFE,     // Position  Mask
  MODID,      // ID
  {0x00, 0x00, 0x00},
  0x00000000, // Reserved
  0x00000000, // Reserved
  {
    // Command Table
    {'T', 'E', 'S', 'T'},
    { 0,  0,  0,  0 },
    { 0,  0,  0,  0 },
    { 0,  0,  0,  0 },
  }
};

COMMON_API *gCommonApi = (COMMON_API *)COMMON_API_BASE;

const char * const gCmdHelper[CMD_NUM] = {
  "                      ;Test",
  0,
  0,
  0,
};

void Main();

__attribute__((section (".entry")))
int ModuleMain ()
{
  BYTE  i, j;
  BYTE  cmd;

  gCommonApi = (COMMON_API *)COMMON_API_BASE;
  cmd = gCommonApi->cmdidx & 0x0F;
  if (cmd == 0x0F) {
    for (i = 0; i < CMD_NUM; i++) {
      if (gModuleHeader.command[i][0]) {
        for (j = 0; j < CMD_LEN; j++) {
          putchar (gModuleHeader.command[i][j]);
        }
        printf ("  %s\n", gCmdHelper[i]);
      }
    }
  } else {
    ZeroBss ();
    Main ();
  }

  return 1;
}

volatile UINT32 SysTickCnt;

void SysTickIrq(void)
{
  // interrupt every 10ms
  SysTickCnt++;
}

void SysTickInit(void)
{
  // t = v / 72K = 10ms
  SysTickCnt = 0;
  SET_ISR  (Vect_SysTick_Handler,  SysTickIrq);
  SysTick_Config (720000);
}

void Main ()
{
  SysTickInit ();

  printf ("Quit\n");

  return;
}

