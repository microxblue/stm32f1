#include "CommonMod.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_spi.h"

#define LED_A_ON         GpioSetBits(GPIOC, GPIO_Pin_13)
#define LED_A_OFF        GpioClrBits(GPIOC, GPIO_Pin_13)
#define LED_A_STS        GpioGetBits(GPIOC, GPIO_Pin_13)

#define LED_B_ON         GpioSetBits(GPIOB, GPIO_Pin_9)
#define LED_B_OFF        GpioClrBits(GPIOB, GPIO_Pin_9)
#define LED_B_STS        GpioGetBits(GPIOB, GPIO_Pin_9)


/*
     MI MO     GD VC CS  CK
  +-------------------------------+
   \ o  o  o | o  o  o | o  o  o /
    +---------------------------+
*/

// 0: bit[7:0] signature low
// 1: bit[7:0] signature high
// 2: bit[3:0] module ID (0-F)
//    bit[4]   command interface  0:has  1:none
// 3: bit[7:0] board support bit map

/*
  Analogue Controller in Red Mode
  BYTE    CMND    DATA
  01     0x01   idle
  02     0x42   0x79
  03     idle    0x5A    Bit0 Bit1 Bit2 Bit3 Bit4 Bit5 Bit6 Bit7
  04     idle    data    SLCT JOYR JOYL STRT UP   RGHT DOWN LEFT
  05     idle    data    L2   R2   L1   R1   /   O    X    |_|
  06     idle    data    Right Joy 0x00 = Left  0xFF = Right
  07     idle    data    Right Joy 0x00 = Up    0xFF = Down
  08     idle    data    Left Joy  0x00 = Left  0xFF = Right
  09     idle    data    Left Joy  0x00 = Up    0xFF = Down

  All Buttons active low.
*/

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
    {'P', 'S', '2', 'G'},
    {'T', 'S', 'T', '1'},
    {'T', 'S', 'T', '2'},
    {'T', 'S', 'T', '3'},
  }
};

COMMON_API *gCommonApi = (COMMON_API *)COMMON_API_BASE;

const char * const gCmdHelper[CMD_NUM] = {
  "                      ;PS2  0 Help", //TST0
  "                      ;Test 1 Help", //TST1
  "                      ;Test 2 Help", //TST2
  "                      ;Test 3 Help", //TST3
};

void Ps2Recv ();

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
    Ps2Recv ();
  }

  return 1;
}


#define TRUE   1
#define FALSE  0

#define CHECK_MARK(e, t)  ((SysTickCnt ^ (Marker.Event##e<<(t))) & (1<<(t)))
#define TOGGLE_MARK(e)    (Marker.Event##e ^= 1)

#define DelayUs(us) do {\
	asm volatile (	"MOV R0,%[loops]\n\t"\
			"1: \n\t"\
			"SUB R0, #1\n\t"\
			"CMP R0, #0\n\t"\
			"BNE 1b \n\t" : : [loops] "r" (9*us) : "memory"\
		      );\
  } while(0)

typedef struct {
    UINT32  Event0 :  1;
    UINT32  Event1 :  1;
    UINT32  Event2 :  1;
    UINT32  Event3 :  1;
    UINT32  Unused : 28;
} TICK_MARKER;

volatile UINT32 SysTickCnt;

void SysTickIrq(void)
{
  // interrupt every 10ms
  SysTickCnt++;
}

void SysTickInit(void)
{
  // t = v / 72K = 10ms
  SET_ISR  (Vect_SysTick_Handler,  SysTickIrq);
  SysTick_Config (720000);
}

void DelayMs(WORD ms)
{
  volatile WORD i, j;
  for(; ms>0; ms--)
          for (i = 0; i < 685; i++)
                  for (j = 0; j < 5; j++);
}

int memcmp(void *s1, void *s2, size_t n)
{
  register unsigned char *str1 = (unsigned char *)s1;
  register unsigned char *str2 = (unsigned char *)s2;
  while (n--)  {
    if (*str1++ != *str2++)
      return 1;
  }
  return 0;
}

void *memset(void *s, int c, size_t n)
{
  register unsigned char *dst = (unsigned char *)s;
  while (n--)  *dst++ = (unsigned char)c;
  return s;
}

#define  GpioGetBits(GPIOx, GPIO_Pin)   (GPIOx->IDR & GPIO_Pin)
#define  GpioSetBits(GPIOx, GPIO_Pin)   (GPIOx->BSRR = GPIO_Pin)
#define  GpioClrBits(GPIOx, GPIO_Pin)   (GPIOx->BRR  = GPIO_Pin)
#define  CR1_CLEAR_Mask                 ((uint16_t)0x3040)
#define  SpiWriteByte                   spiwrite
#define  CS_LOW                         GpioClrBits(GPIOB, GPIO_Pin_12)
#define  CS_HIGH                        GpioSetBits(GPIOB,   GPIO_Pin_12)

const BYTE PollState[]   = {0x01,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const BYTE EnterConfig[] = {0x01,0x43,0x00,0x01,0x00};
const BYTE SetMode[]     = {0x01,0x44,0x00,0x01,0x03,0x00,0x00,0x00,0x00};
const BYTE ExitConfig[]  = {0x01,0x43,0x00,0x00,0x5A,0x5A,0x5A,0x5A,0x5A};

// It is important to keep the byte and frame delay
void PreDelay (WORD ms)
{
  volatile WORD i;
  for(; ms>0; ms--) for (i = 0; i < 1; i++);
}

#define DELAY_BYTE_US  25
void SendFrame (const BYTE *Cmd, BYTE Len, BYTE *Buf)
{
  BYTE Data, Idx;

  PreDelay (1);

  CS_LOW;
  DelayUs (DELAY_BYTE_US);
  for (Idx = 0; Idx < Len; Idx++) {
    Data = SpiWriteByte(Cmd[Idx]);
    if (Buf) {
      Buf[Idx] = Data;
    }
    DelayUs (DELAY_BYTE_US);
  }
  CS_HIGH;
}

void Ps2Init ()
{
  UINT16           TmpReg;
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


// 0: Digital  1: Analog
#define  PS2_MODE  1

BYTE PollPs2 (BYTE *DatBuf) {
  BYTE  Blen;
  Blen = PS2_MODE ? 9 : 5;
  SendFrame (PollState, Blen, DatBuf);
  if (DatBuf[2] == 0x5A) {
    return TRUE;
  } else {
    return TRUE;
  }
}

void Ps2Recv ()
{
  TICK_MARKER  Marker;
  BYTE         DatBuf[2][9];
  BYTE        *DatPtr;
  BYTE         Idx;
  BYTE         ActIdx;
  BYTE         ModeState;

  memset (&Marker, 0, sizeof(TICK_MARKER));

  ActIdx     = 0;
  ModeState  = 0;
  SysTickCnt = 0;
  SysTickInit ();

  // Force to analog mode
  Ps2Init ();

  while (TRUE) {

    if (CHECK_MARK(0, 3)) {
      // 10 * (1<<3) = 80ms
      TOGGLE_MARK(0);
      {
        DatPtr = DatBuf[ActIdx];
        if ((ModeState == 0) && PollPs2 (DatPtr)) {
          if (0) {
            printf ("!\n");
            for (Idx = 0; Idx < 9; Idx++) {
                printf ("0x%02X ", DatPtr[Idx]);
            }
            printf ("\n");
          }
          LED_B_ON;
          if (DatPtr[1] != 0x41) {
            ModeState = 0;
          } else {
            if (memcmp (DatBuf[ActIdx], DatBuf[1 - ActIdx], 9) != 0) {
              for (Idx = 0; Idx < 9; Idx++) {
                printf ("0x%02X ", DatPtr[Idx]);
              }
              printf ("\n");
              ActIdx = 1 - ActIdx;
            }
          }
          LED_B_OFF;
        }
      }
    }

    if (CHECK_MARK(1, 1)) {
      // 10 * (1<<1) = 20ms
      TOGGLE_MARK(1);
      switch (ModeState) {
        case  1:
          SendFrame (EnterConfig, 5, 0);
          ModeState++;
          break;
        case  2:
          SendFrame (SetMode, 9, 0);
          ModeState++;
          break;
        case  3:
          SendFrame (ExitConfig, 9, 0);
          ModeState++;
          break;
        case  4:
          ModeState = 0;
          break;
        default:
          break;
      }
    }

    if (CHECK_MARK(2, 7)) {
      TOGGLE_MARK(2);
      if (LED_A_STS) {
        //LED_A_OFF;
      } else {
        //LED_A_ON;
      }
    }

    if (haschar()) {
      getchar();
      break;
    }

  }

}
