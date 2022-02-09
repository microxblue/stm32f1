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

#define delayUS_ASM(us) do {\
  asm volatile (  "MOV R0,%[loops]\n\t"\
      "1: \n\t"\
      "SUB R0, #1\n\t"\
      "CMP R0, #0\n\t"\
      "BNE 1b \n\t" : : [loops] "r" (9*us) : "memory"\
          );\
  } while(0)

#define  DelayUs      delayUS_ASM

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

void pin_init (void)
{
  GPIO_InitTypeDef         GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  gpioinit (GPIOA, &GPIO_InitStructure);
  GpioClrBits(GPIOA, GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9);
}

void DelayMs (WORD ms)
{
  volatile WORD i, j;
  for (; ms > 0; ms--)
    for (i = 0; i < 685; i++)
      for (j = 0; j < 5; j++);
}


void dir_ctrl ()
{
  GpioClrBits (GPIOA, 0x3FC);
  //GpioSetBits (GPIOA, GPIO_Pin_2|GPIO_Pin_4|GPIO_Pin_7|GPIO_Pin_9);  // F
  //GpioSetBits (GPIOA, GPIO_Pin_3|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_8);  // B
  //GpioSetBits (GPIOA, GPIO_Pin_2|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_9);  // R
  //GpioSetBits (GPIOA, GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_7|GPIO_Pin_8);  // L

  DelayMs (1000);
  GpioClrBits (GPIOA, 0x3FC);

  //GpioSetBits (GPIOA, GPIO_Pin_2);  // LF (F)
  //GpioSetBits (GPIOA, GPIO_Pin_3);  // LF (B)
  //GpioSetBits (GPIOA, GPIO_Pin_4);  // LB (F)
  //GpioSetBits (GPIOA, GPIO_Pin_5);  // LB (B)

  //GpioSetBits (GPIOA, GPIO_Pin_6);  // RF (B)
  //GpioSetBits (GPIOA, GPIO_Pin_7);  // RF (F)
  //GpioSetBits (GPIOA, GPIO_Pin_8);  // RB (B)
  //GpioSetBits (GPIOA, GPIO_Pin_9);  // LB (F)
}

#define  CS_LOW          GpioClrBits(GPIOB, GPIO_Pin_12)
#define  CS_HIGH         GpioSetBits(GPIOB,   GPIO_Pin_12)
#define  CR1_CLEAR_Mask  ((uint16_t)0x3040)

void ps2_init ()
{
  UINT16  TmpReg;
  SPI_InitTypeDef  SPI_InitStruct;

  /* SPI2 configuration */
  SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
  SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStruct.SPI_CPOL = SPI_CPOL_High;
  SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;//2Edge;
  SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
  SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_LSB;
  SPI_InitStruct.SPI_CRCPolynomial = 0;
  TmpReg = SPI2->CR1;
  TmpReg &= CR1_CLEAR_Mask;
  TmpReg |= (uint16_t)((uint32_t)SPI_InitStruct.SPI_Direction | SPI_InitStruct.SPI_Mode |
                  SPI_InitStruct.SPI_DataSize | SPI_InitStruct.SPI_CPOL |
                  SPI_InitStruct.SPI_CPHA | SPI_InitStruct.SPI_NSS |
                  SPI_InitStruct.SPI_BaudRatePrescaler | SPI_InitStruct.SPI_FirstBit);
  SPI2->CR1 = TmpReg;

  CS_HIGH;
}

void PreDelay (WORD ms)
{
  volatile WORD i;
  for(; ms>0; ms--) for (i = 0; i < 1; i++);
}

#define DELAY_BYTE_US   10
#define DELAY_FRAME_MS  18
#define SpiWrite        spiwrite
void ps2_send_frame (const BYTE *Cmd, BYTE Len, BYTE *Buf)
{
  BYTE Data, Idx;

  PreDelay (1);

  CS_LOW;
  DelayUs (DELAY_BYTE_US);
  for (Idx = 0; Idx < Len; Idx++) {
    Data = SpiWrite(Cmd[Idx]);
    if (Buf) {
      Buf[Idx] = Data;
    }
    DelayUs (DELAY_BYTE_US);
  }
  CS_HIGH;
}

#define   LED_A_ON                  GpioSetBits(GPIOC, GPIO_Pin_13)
#define   LED_A_OFF                 GpioClrBits(GPIOC, GPIO_Pin_13)

const BYTE m_poll_state[] = {
  0x01,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

void Main ()
{
  bool         loop;
  bool         ready;
  TICK_MARKER  Marker = {0};
  BYTE         data[5];
  BYTE         idx;
  UINT32       last_tick;

  SysTickInit ();

  pin_init ();

  ps2_init ();

  ready     = false;
  loop      = true;
  last_tick = SysTickCnt;
  while (loop) {

    if (SysTickCnt < last_tick) {
      last_tick = SysTickCnt;
    } else if (SysTickCnt - last_tick > 10) {
      ps2_send_frame (m_poll_state, 5, data);
      last_tick = SysTickCnt;
      ready = true;
    }

    if (ready) {
      ready = false;
      if (data[2] == 0x5A) {

        if (0) {
          for (idx = 0; idx < 5; idx++) {
            printf ("0x%02X ", data[idx]);
          }
          printf ("\n");
          data[4] = 0xF0;
        }

        if ((data[3] & 0xF0) != 0xF0) {
          if ((data[3] & 0x80) == 0) {
            GpioClrBits (GPIOA, 0x3FC);
            GpioSetBits (GPIOA, GPIO_Pin_3|GPIO_Pin_5|GPIO_Pin_7|GPIO_Pin_9);  // F
          }
          if ((data[3] & 0x20) == 0) {
            GpioClrBits (GPIOA, 0x3FC);
            GpioSetBits (GPIOA, GPIO_Pin_2|GPIO_Pin_4|GPIO_Pin_6|GPIO_Pin_8);  // B
          }
        }

        if ((data[4] & 0xF0) != 0xF0) {
          if ((data[4] & 0x90) == 0) {
            // LF
            GpioClrBits (GPIOA, 0x3FC);
            GpioSetBits (GPIOA, GPIO_Pin_4|GPIO_Pin_7);
          } else if ((data[4] & 0x30) == 0) {
            // RF
            GpioClrBits (GPIOA, 0x3FC);
            GpioSetBits (GPIOA, GPIO_Pin_2|GPIO_Pin_9);
          } else if ((data[4] & 0xC0) == 0) {
            // LB
            GpioClrBits (GPIOA, 0x3FC);
            GpioSetBits (GPIOA, GPIO_Pin_3|GPIO_Pin_8);
          } else if ((data[4] & 0x60) == 0) {
            // RB
            GpioClrBits (GPIOA, 0x3FC);
            GpioSetBits (GPIOA, GPIO_Pin_5|GPIO_Pin_6);
          } else if ((data[4] & 0x10) == 0) {
            // Forward
            GpioClrBits (GPIOA, 0x3FC);
            GpioSetBits (GPIOA, GPIO_Pin_2|GPIO_Pin_4|GPIO_Pin_7|GPIO_Pin_9);  // F
          } else if ((data[4] & 0x20) == 0) {
            // Right
            GpioClrBits (GPIOA, 0x3FC);
            GpioSetBits (GPIOA, GPIO_Pin_2|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_9);  // R
          } else if ((data[4] & 0x40) == 0) {
            // Back
            GpioClrBits (GPIOA, 0x3FC);
            GpioSetBits (GPIOA, GPIO_Pin_3|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_8);  // B
          } else if ((data[4] & 0x80) == 0) {
            // Left
            GpioClrBits (GPIOA, 0x3FC);
            GpioSetBits (GPIOA, GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_7|GPIO_Pin_8);  // L
          }
        }

        if ((data[3] & data[4]) == 0xFF) {
          GpioClrBits (GPIOA, 0x3FC);
        }

      }
    }

    if (SysTickCnt & 0x20) {
      LED_A_ON;
    } else {
      LED_A_OFF;
    }

    if (haschar()) {
      (void)getchar();
      loop = false;
    }
  }

  GpioClrBits (GPIOA, 0x3FC);
  printf ("Quit\n");

  return;
}

