#ifndef _MODULE_API_H_
#define _MODULE_API_H_

#include "Common.h"

#define   KB(x)             (x<<10)
#define   MODULE_SIZE       KB(8)
#define   SYS_SRAM_BASE     (0x20000000)

#define   SPARE_SRAM_BASE   (SYS_SRAM_BASE + KB(19))
#define   COMMON_API_BASE   (SPARE_SRAM_BASE + KB(0))
#define   ISR_VECT_BASE     (COMMON_API_BASE + 0x0300)
#define   USER_REG_BASE     (COMMON_API_BASE + 0x03F0)

#define   MODULE_ENTRY_ROM  (0x08018000  + sizeof(MODULE_HEADER) + 1)
#define   MODULE_BASE       (SYS_SRAM_BASE + KB(4))
#define   MODULE_ENTRY      (MODULE_BASE + sizeof(MODULE_HEADER) + 1)

#define   MODULE_SLOT_NUM   (0x10000/MODULE_SIZE)
#define   MODULE_SLOT_MASK  ((1 << MODULE_SLOT_NUM) - 1)
#define   MODULE_COUNT      16
#define   MODULE_COUNT_MASK (MODULE_COUNT - 1)
#define   MODULE_BLK_BASE   0x70

#define   SET_ISR(x, y)     *(unsigned int *)(ISR_VECT_BASE + (4*(x))) = ((unsigned int)(y))

#define   Vect_Stack_Top                         (00)
#define   Vect_Reset_Handler                     (01)
#define   Vect_NMI_Handler                       (02)
#define   Vect_HardFault_Handler                 (03)
#define   Vect_MemManage_Handler                 (04)
#define   Vect_BusFault_Handler                  (05)
#define   Vect_UsageFault_Handler                (06)
#define   Vect_SVC_Handler                       (11)
#define   Vect_DebugMon_Handler                  (12)
#define   Vect_PendSV_Handler                    (14)
#define   Vect_SysTick_Handler                   (15)

#define   Vect_IRQHandler(x)                     ((x) + 16)

// #define   Vect_WWDG_IRQHandler                   (16)
// #define   Vect_PVD_IRQHandler                    (17)
// #define   Vect_TAMP_STAMP_IRQHandler             (18)
// #define   Vect_RTC_WKUP_IRQHandler               (19)
// #define   Vect_FLASH_IRQHandler                  (20)
// #define   Vect_RCC_IRQHandler                    (21)
// #define   Vect_EXTI0_IRQHandler                  (22)
// #define   Vect_EXTI1_IRQHandler                  (23)
// #define   Vect_EXTI2_IRQHandler                  (24)
// #define   Vect_EXTI3_IRQHandler                  (25)
// #define   Vect_EXTI4_IRQHandler                  (26)
// #define   Vect_DMA1_Stream1_IRQHandler           (27)
// #define   Vect_DMA1_Stream2_IRQHandler           (28)
// #define   Vect_DMA1_Stream3_IRQHandler           (29)
// #define   Vect_DMA1_Stream4_IRQHandler           (30)
// #define   Vect_DMA1_Stream5_IRQHandler           (31)
// #define   Vect_DMA1_Stream6_IRQHandler           (32)
// #define   Vect_DMA1_Stream7_IRQHandler           (33)
// #define   Vect_ADC_IRQHandler                    (34)
// #define   Vect_USB_HP_CAN1_TX_IRQHandler         (35)
// #define   Vect_USB_LP_CAN1_RX0_IRQHandler        (36)
// #define   Vect_CAN1_RX1_IRQHandler               (37)
// #define   Vect_CAN1_SCE_IRQHandler               (38)
// #define   Vect_EXTI9_5_IRQHandler                (39)
// #define   Vect_TIM1_BRK_TIM9_IRQHandler          (40)
// #define   Vect_TIM1_UP_TIM10_IRQHandler          (41)
// #define   Vect_TIM1_TRG_COM_TIM11_IRQHandler     (42)
// #define   Vect_TIM1_CC_IRQHandler                (43)
// #define   Vect_TIM2_IRQHandler                   (44)
// #define   Vect_TIM3_IRQHandler                   (45)
// #define   Vect_TIM4_IRQHandler                   (46)
// #define   Vect_I2C1_EV_IRQHandler                (47)
// #define   Vect_I2C1_ER_IRQHandler                (48)
// #define   Vect_I2C2_EV_IRQHandler                (49)
// #define   Vect_I2C2_ER_IRQHandler                (50)
// #define   Vect_SPI1_IRQHandler                   (51)
// #define   Vect_SPI2_IRQHandler                   (52)
// #define   Vect_USART1_IRQHandler                 (53)
// #define   Vect_USART2_IRQHandler                 (54)
// #define   Vect_USART3_IRQHandler                 (55)
// #define   Vect_EXTI15_10_IRQHandler              (56)
// #define   Vect_RTC_Alarm_IRQHandler              (57)

typedef int   (*API_MODENTRY)();
typedef void  (*API_PRINTF)(const char *, ...);
typedef char  (*API_HASCHAR)();
typedef char  (*API_GETCHAR)();
typedef void  (*API_PUTCHAR)(char c);
typedef BYTE *(*API_USB_GET_RX_BUF)(WORD *len);
typedef void  (*API_USB_FREE_RX_BUF)();
typedef BYTE *(*API_USB_GET_TX_BUF)(WORD len);
typedef void  (*API_USB_ADD_TX_BUF)(WORD len);
typedef void  (*API_GPIO_INIT)(void *gpio, void *init);
typedef void  (*API_NVIC_INIT)(void *nvic);
typedef void  (*API_UART_INIT)(void *nart, void *init);
typedef BYTE  (*API_SPI_WRITE)(BYTE val);
typedef BYTE  (*API_SET_CON)(BYTE val);

#define STRUCT_OFF(s,f)   (UINT32)(&((s *)0)->f)

#define  CMD_LEN    4
#define  CMD_NUM    4

#pragma pack(1)
typedef struct {
  unsigned short  marker;
  unsigned short  posmask;
  unsigned char   id;
  unsigned char   unused[3];
  unsigned int    reserved2;
  unsigned int    reserved3;
  unsigned char   command[CMD_NUM][CMD_LEN];
} MODULE_HEADER;

typedef struct {
  // offset 0x000
  union {
    unsigned char  param_c;
    unsigned short param_s;
    unsigned long  param_l;
  } params[4];

  // offset 0x010
  unsigned char        paramcnt;
  unsigned char        cmdidx;
  unsigned char        currpage;
  unsigned char        boardid;

  // offset 0x014
  unsigned int         romstart;
  unsigned int         romend;
  unsigned int         status[2];

  // offset 0x024
  API_HASCHAR          HasChar;
  API_GETCHAR          GetChar;
  API_PUTCHAR          PutChar;
  API_PRINTF           Printf;

  // offset 0x034
  API_USB_GET_RX_BUF   UsbGetRxBuf;
  API_USB_FREE_RX_BUF  UsbFreeRxBuf;
  API_USB_GET_TX_BUF   UsbGetTxBuf;
  API_USB_ADD_TX_BUF   UsbAddTxBuf;

  // offset 0x044
  API_GPIO_INIT        GpioInit;
  API_SPI_WRITE        SpiWrite;
  API_NVIC_INIT        NvicInit;
  API_UART_INIT        UartInit;

  // offset 0x054
  API_SET_CON          SetCon;
  API_SET_CON          Reserved[3];

  // Must not exceed offset 0x300
  // Since it is used as  ISRBASE
} COMMON_API;
#pragma pack()

extern COMMON_API    *gCommonApi;
extern API_MODENTRY   gModEntry;

#endif


