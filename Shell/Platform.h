#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#if (PINS == 48)
#define  PACKAGE_PINS                       "48"
#define  BOARD_ID                           0xF0

#define  USB_DISCONNECT                     GPIOA
#define  USB_DISCONNECT_PIN                 GPIO_Pin_15
#define  RCC_APB2Periph_GPIO_DISCONNECT     RCC_APB2Periph_GPIOA

#define  LED_B_ON                           GPIO_SetBits(GPIOB,   GPIO_Pin_9)
#define  LED_B_OFF                          GPIO_ResetBits(GPIOB, GPIO_Pin_9)
#define  LED_A_ON                           GPIO_SetBits(GPIOC,   GPIO_Pin_13)
#define  LED_A_OFF                          GPIO_ResetBits(GPIOC, GPIO_Pin_13)

#define  CS_LOW                             GPIOA->BRR  = (1 << 4)
#define  CS_HIGH                            GPIOA->BSRR = (1 << 4)
#define  ONBOARD_SPI                        SPI1
#define  EXTERNAL_SPI                       SPI2

#define  START_CMD                          "DPRG 0 0"

// PIN48
//
//  1-PA3:IO1       2-PA2:IO4
//  3-PA1:IO2/CS2   4-PA0:NC
//  5-VCC           6-GND
//  7-PB12:CS       8-PB13:CLK
//  9-PB14:MISO    10-PB15:MOSI
// 11-PB1:VPP      12-PB0:IO3
// 13-PB10:SCL     14-PB11:SDA
//

#define  GPIOA_SET                          GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_15
#define  GPIOA_CLR                          0
#define  GPIOB_SET                          GPIO_Pin_0 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_12
#define  GPIOB_CLR                          GPIO_Pin_9 | GPIO_Pin_1
#define  GPIOC_SET                          0
#define  GPIOC_CLR                          GPIO_Pin_13
#define  GPIOD_SET                          0
#define  GPIOD_CLR                          0

/* Configure SPI CS (PA.4) and USB Connect JTDI (PA.15) as push-pull */
/* Configure JTDO/JTRST (PB.3/PB.4) as push-pull */
/* Configure LEDB (PB.9) as push-pull */
/* Configure LEDA (PC.13) as push-pull */

#elif (PINS == 64)

#define  PACKAGE_PINS                       "64"
#define  BOARD_ID                           0xF1

#define  USB_DISCONNECT                     GPIOD
#define  USB_DISCONNECT_PIN                 GPIO_Pin_2
#define  RCC_APB2Periph_GPIO_DISCONNECT     RCC_APB2Periph_GPIOD

#define  LED_B_ON                           GPIO_SetBits(GPIOB,   GPIO_Pin_0)
#define  LED_B_OFF                          GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#define  LED_A_ON                           GPIO_SetBits(GPIOC,   GPIO_Pin_13)
#define  LED_A_OFF                          GPIO_ResetBits(GPIOC, GPIO_Pin_13)

#define  CS_LOW                             GPIOB->BRR  = (1 << 12)
#define  CS_HIGH                            GPIOB->BSRR = (1 << 12)
#define  ONBOARD_SPI                        SPI2
#define  EXTERNAL_SPI                       SPI1

#define  START_CMD                          ""

// PIN64
//
//  1-PA10:IO1      2-PA9:IO4
//  3-PA8:IO2/CS2   4-GND
//  5-VCC           6-GND
//  7-PA4:CS        8-PA5:CLK
//  9-PA6:MISO     10-PA7:MOSI
// 11-PB7:VPP      12-PB6:IO3
// 13-PB8:SCL      14-PB9:SDA
//

#define  GPIOA_SET                          GPIO_Pin_4 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10
#define  GPIOA_CLR                          0
#define  GPIOB_SET                          GPIO_Pin_1 | GPIO_Pin_6 | GPIO_Pin_7
#define  GPIOB_CLR                          GPIO_Pin_0
#define  GPIOC_SET                          0
#define  GPIOC_CLR                          GPIO_Pin_13
#define  GPIOD_SET                          GPIO_Pin_2
#define  GPIOD_CLR                          0

/* Configure LED0 (PB.0) as push-pull */
/* Configure JTAG VCC (PB.1) as push-pull */
/* Configure IO1 (PB.6) as push-pull */
/* Configure IO2 (PB.7) as push-pull */
/* Configure LED0 (PC.13) as push-pull */
/* Configure USB Connect (PD.2) as push-pull */

#else

#error ('PINS must be defined to either 48 or 64 !')

#endif

#endif