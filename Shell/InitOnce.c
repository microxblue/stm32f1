#include "Common.h"
#include "stm32f10x_conf.h"
#include "usb_lib.h"
#include "Console.h"
#include "ModuleApi.h"

void ModInit ();

void GpioInit ()
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* JTAG-DP Disabled and SW-DP Enabled */
  RCC_APB2PeriphClockCmd (RCC_APB2Periph_AFIO, ENABLE);
  GPIO_PinRemapConfig (GPIO_Remap_SWJ_JTAGDisable, ENABLE);

  /* Enable GPIO A/B/C/D clock */
  RCC_APB2PeriphClockCmd(
    RCC_APB2Periph_GPIOA | \
    RCC_APB2Periph_GPIOB | \
    RCC_APB2Periph_GPIOC | \
    RCC_APB2Periph_GPIOD,  \
    ENABLE);

  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Pin = (GPIOA_SET | GPIOA_CLR);
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_SetBits(GPIOA, GPIOA_SET);
  GPIO_ResetBits(GPIOA, GPIOA_CLR);

  GPIO_InitStructure.GPIO_Pin = (GPIOB_SET | GPIOB_CLR);
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_SetBits(GPIOB, GPIOB_SET);
  GPIO_ResetBits(GPIOB, GPIOB_CLR);

  GPIO_InitStructure.GPIO_Pin = (GPIOC_SET | GPIOC_CLR);
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_SetBits(GPIOC, GPIOC_SET);
  GPIO_ResetBits(GPIOC, GPIOC_CLR);

  GPIO_InitStructure.GPIO_Pin = (GPIOD_SET | GPIOD_CLR);
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  GPIO_SetBits(GPIOD, GPIOD_SET);
  GPIO_ResetBits(GPIOD, GPIOD_CLR);

  /* Set PA13/14 as input */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_14 ;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void UartInit ()
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  /* Enable UART clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

  /* Configure USART1 Rx (PA.10) as input floating */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Configure USART1 Tx (PA.09) as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* USART resources configuration (Clock, GPIO pins and USART registers) ----*/
  /* USART configured as follow:
        - BaudRate = 115200 baud
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  /* USART configuration */
  USART_Init(USART1, &USART_InitStructure);

  /* Enable the USART1 */
  USART_Cmd(USART1, ENABLE);
}

void TimerInit(void)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  NVIC_InitTypeDef         NVIC_InitStructure;

  /* TIM3 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

  /* Enable the TIM2 gloabal Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Time base configuration */
  //TIM_TimeBaseStructure.TIM_Period = 10000;  //  x * 10K   = 1s
  TIM_TimeBaseStructure.TIM_Period = 500;      //  x * 10K   = 1s
  TIM_TimeBaseStructure.TIM_Prescaler = 7199;  //  72M/(x+1) = 10K
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

  /* TIM IT enable */
  TIM_ClearFlag(TIM2, TIM_FLAG_Update);
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

  /* Enable counter */
  TIM_Cmd(TIM2, ENABLE);

  /* Timer 3 & 4 */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

  /* Enable SysTick at 72M */
  SysTick->CTRL  = 0;
  SysTick->LOAD  = 0xFFFFFF;
  SysTick->VAL   = 0;
  SysTick->CTRL  = SysTick_CLKSource_HCLK | SysTick_CTRL_ENABLE_Msk;
}

void SpiInit ()
{
  SPI_InitTypeDef  SPI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  // SPI1
  // =======================================================================
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

  // Input PA4 as output
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // SPI  Deselected
  GPIO_SetBits(GPIOA, GPIO_Pin_4);

  /* Configure SPI1 pins: MISO, SCK and MOSI */
  GPIO_InitStructure.GPIO_Pin=GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* SPI1 configuration */
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;

  SPI_Init(SPI1, &SPI_InitStructure);

  /* Enable SPI1  */
  SPI_Cmd(SPI1, ENABLE);

  // SPI2
  // =======================================================================
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

#if USER_FLAGS == 1
  /* For USB power control PB12/13/14/15 */
  GPIO_SetBits(GPIOB, GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
  GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_OD;
  GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init( GPIOB, &GPIO_InitStructure);
#else
  // CS PB12 as output
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  // SPI  Deselected
  GPIO_SetBits(GPIOB, GPIO_Pin_12);

  /* Configure SPI2 pins: MISO, SCK and MOSI */
  GPIO_InitStructure.GPIO_Pin=GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init( GPIOB, &GPIO_InitStructure);
#endif

  /* SPI2 configuration */
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;

  SPI_Init(SPI2, &SPI_InitStructure);

  /* Enable SPI2  */
  SPI_Cmd(SPI2, ENABLE);
}

void DelayMs(WORD ms)
{
  volatile WORD i, j;
  for(; ms>0; ms--)
          for (i = 0; i < 500; i++)
                  for (j = 0; j < 5; j++);
}

void UsbDisconnect (WORD ms)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_ResetBits(GPIOA, GPIO_Pin_12);
  DelayMs (ms);
}

void UsbInit ()
{
  NVIC_InitTypeDef NVIC_InitStructure;
  extern   uint8_t Device_DeviceDescriptor[];

  /* Update device ID basing on PB2 (BOOT1) */
  if (GPIOB->IDR & GPIO_Pin_2) {
    Device_DeviceDescriptor[10] = 0x25;
    Device_DeviceDescriptor[11] = 0x09;
  }

#if USER_FLAGS == 1
  Device_DeviceDescriptor[10] = 0x18;
  Device_DeviceDescriptor[11] = 0x09;
#endif

  /* Select USBCLK source */
  RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);

  /* Enable the USB clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);

  // Workaround for Blue-Pill (No USB disconnect control pin)
  UsbDisconnect (5);

  // USB Disconnected
  GPIO_SetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);

  // Setup USB interrupt
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  USB_Init();
}

void SysInit ()
{
  SystemInit ();
  GpioInit   ();
  UartInit   ();
  TimerInit  ();
  SpiInit    ();
  UsbInit    ();
  ModInit    ();

#if ISR_VECT_IN_RAM
  memcpy ((void *)ISR_VECT_BASE, (void *)VECT_TAB_BASE, 0x100);
  SCB->VTOR = ISR_VECT_BASE;
#endif

  SetCon (SERIAL_CON);
}