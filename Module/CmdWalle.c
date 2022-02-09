#include "CommonMod.h"
#include "stm32f10x_conf.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_spi.h"
#include "usb_regs.h"

#define TRUE   1
#define FALSE  0

#define MG996R_MIN   0

#define   STR__CAT(a,b)             a ## b
#define   STR_CAT(a,b)              STR__CAT(a,b)
#define   STR__CAT3(a,b,c)          a ## b ## c
#define   STR_CAT3(a,b,c)           STR__CAT3(a,b,c)
#define   GPIO_PIN(x)               STR_CAT(GPIO_Pin_, x)
#define   GPIO_PORT(x)              STR_CAT(GPIO, x)
#define   SET_GPIO(x)               GPIO_PORT(x##_PORT)->BRR  = GPIO_PIN(x##_PIN)
#define   CLR_GPIO(x)               GPIO_PORT(x##_PORT)->BSRR = GPIO_PIN(x##_PIN)
#define   GET_GPIO(x)               (GPIO_PORT(x##_PORT)->IDR  & GPIO_PIN(x##_PIN))
#define   EnableExtInterrupt(Pin)   {EXTI->PR |= GPIO_PIN(Pin##_PIN); EXTI->IMR |= GPIO_PIN(Pin##_PIN);}
#define   DisableExtInterrupt(Pin)  {EXTI->IMR &= ~GPIO_PIN(Pin##_PIN); EXTI->PR |= GPIO_PIN(Pin##_PIN);}
#define   IsInterruptEnabled(Pin)   (EXTI->IMR & GPIO_PIN(Pin##_PIN))

#define   LED_A_ON                  GPIO_SetBits(GPIOC,   GPIO_Pin_13)
#define   LED_A_OFF                 GPIO_ResetBits(GPIOC, GPIO_Pin_13)
#define   EP_MAX_SIZE               64
#define   CMD_FRAME_LONG_LEN        (CMD_PACKET_LEN + TX_DATA_WITDH)

#undef    printf
#define   printf(fmt, args...)      if (PrtOn) gCommonApi->Printf(fmt, ##args)
#define   IsUsbCablePlugged()       (_GetFNR() & FNR_LCK)

#define DefExtiInit(Pin) \
void ExtiInit_##Pin (BYTE PreemptionPriority, BYTE SubPriority)  \
{ \
  NVIC_InitTypeDef NVIC_InitStructure; \
  /* Select EXT interrupt */ \
  AFIO->EXTICR[Pin##_PIN / 4] |= STR_CAT (AFIO_EXTICR1_EXTI0_P, Pin##_PORT) << ((Pin##_PIN % 4) << 2); \
  /* Clear Rising Falling edge configuration */ \
  EXTI->RTSR  |=  GPIO_PIN(Pin##_PIN); \
  EXTI->FTSR   =  0; \
  /* Mask interrupt */ \
  DisableExtInterrupt (Pin); \
  /* Enable and set EXTI Interrupt */ \
  NVIC_InitStructure.NVIC_IRQChannel = Pin##_IRQ; \
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = PreemptionPriority; \
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = SubPriority; \
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; \
  nvicinit(&NVIC_InitStructure); \
}

#define delayUS_ASM(us) do {\
  asm volatile (  "MOV R0,%[loops]\n\t"\
      "1: \n\t"\
      "SUB R0, #1\n\t"\
      "CMP R0, #0\n\t"\
      "BNE 1b \n\t" : : [loops] "r" (9*us) : "memory"\
          );\
  } while(0)


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
    {'T', 'S', 'T', '0'},
    {'T', 'S', 'T', '1'},
    {'T', 'S', 'T', '2'},
    {'T', 'S', 'T', '3'},
  }
};

COMMON_API *gCommonApi = (COMMON_API *)COMMON_API_BASE;

const char * const gCmdHelper[CMD_NUM] = {
  "                      ;Test 0 Help", //TST0
  "                      ;Test 1 Help", //TST1
  "                      ;Test 2 Help", //TST2
  "                      ;Test 3 Help", //TST3
};

#define  LED_SIZE    16
DWORD Led[LED_SIZE + 1] = {
  //xxGGRRBB
  0x80FF0000,
  0x80000000,
  0x80000000,
  0x80000000,

  0x80000000,
  0x80000000,
  0x80000000,
  0x80000000,

  0x80000000,
  0x80000000,
  0x80000000,
  0x80000000,

  0x80000000,
  0x80000000,
  0x80000000,
  0x80000000,
};

UINT8  Debug  = 1;
UINT8  PrtOn  = 1;

void Walle();
void RefreshLed ();
void SetLedColor (BYTE Idx, DWORD Rgb);

__attribute__((section (".entry")))
int ModuleMain ()
{
  BYTE  i, j;
  BYTE  cmd;

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
    if (gCommonApi->paramcnt < 2) {
      if (gCommonApi->params[1].param_c == 0xFF) {
        for (i = 0; i < 16; i++) {
          printf ("\n");
          for (j = 0; j < 3; j++) {
            printf ("%02X ", Led[i * 3 + j]);
          }
        }
        printf ("\n");
      } else {
        Walle ();
      }
    }
  }

  return 1;
}

void *memcpy(void *d, void *s, size_t n)
{
  register unsigned char *dst = (unsigned char *)d;
  register unsigned char *src = (unsigned char *)s;
  while (n--)  *dst++ = *src++;
  return d;
}

volatile UINT32 SysTickCnt;

UINT32 GetSysTick (void)
{
  DWORD  Val1;
  DWORD  Val2;
  DWORD  Tick;

  Val1 = SysTickCnt;
  Tick = SysTick->VAL;
  Val2 = SysTickCnt;
  if (Val1 == Val2) {
    // No over flow
    Tick = (Val1<<20) | ((0xFFFFF - Tick) >> 8);
  } else {
    // over flow occured, read again
    Tick = (SysTickCnt<<20) | ((0xFFFFF - SysTick->VAL) >> 8);
  }

  return Tick;
}


void DelayMs (WORD ms)
{
  volatile WORD i, j;
  for (; ms > 0; ms--)
    for (i = 0; i < 685; i++)
      for (j = 0; j < 5; j++);
}

/**
  * @brief  Sets the selected data port bits.
  * @param  GPIOx: where x can be (A..G) to select the GPIO peripheral.
  * @param  GPIO_Pin: specifies the port bits to be written.
  *   This parameter can be any combination of GPIO_Pin_x where x can be (0..15).
  * @retval None
  */
void GPIO_SetBits (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
  GPIOx->BSRR = GPIO_Pin;
}

/**
  * @brief  Clears the selected data port bits.
  * @param  GPIOx: where x can be (A..G) to select the GPIO peripheral.
  * @param  GPIO_Pin: specifies the port bits to be written.
  *   This parameter can be any combination of GPIO_Pin_x where x can be (0..15).
  * @retval None
  */
void GPIO_ResetBits (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
  GPIOx->BRR = GPIO_Pin;
}

void TIM_TimeBaseInit(TIM_TypeDef* TIMx, TIM_TimeBaseInitTypeDef* TIM_TimeBaseInitStruct)
{
  uint16_t tmpcr1 = 0;

  tmpcr1 = TIMx->CR1;

  if((TIMx == TIM1) || (TIMx == TIM8)|| (TIMx == TIM2) || (TIMx == TIM3)||
     (TIMx == TIM4) || (TIMx == TIM5))
  {
    /* Select the Counter Mode */
    tmpcr1 &= (uint16_t)(~((uint16_t)(TIM_CR1_DIR | TIM_CR1_CMS)));
    tmpcr1 |= (uint32_t)TIM_TimeBaseInitStruct->TIM_CounterMode;
  }

  if((TIMx != TIM6) && (TIMx != TIM7))
  {
    /* Set the clock division */
    tmpcr1 &= (uint16_t)(~((uint16_t)TIM_CR1_CKD));
    tmpcr1 |= (uint32_t)TIM_TimeBaseInitStruct->TIM_ClockDivision;
  }

  TIMx->CR1 = tmpcr1;

  /* Set the Autoreload value */
  TIMx->ARR = TIM_TimeBaseInitStruct->TIM_Period ;

  /* Set the Prescaler value */
  TIMx->PSC = TIM_TimeBaseInitStruct->TIM_Prescaler;

  if ((TIMx == TIM1) || (TIMx == TIM8)|| (TIMx == TIM15)|| (TIMx == TIM16) || (TIMx == TIM17))
  {
    /* Set the Repetition Counter value */
    TIMx->RCR = TIM_TimeBaseInitStruct->TIM_RepetitionCounter;
  }

  /* Generate an update event to reload the Prescaler and the Repetition counter
     values immediately */
  TIMx->EGR = TIM_PSCReloadMode_Immediate;
}

void TIM_ClearFlag(TIM_TypeDef* TIMx, uint16_t TIM_FLAG)
{
  /* Clear the flags */
  TIMx->SR = (uint16_t)~TIM_FLAG;
}

void TIM_Cmd(TIM_TypeDef* TIMx, FunctionalState NewState)
{
  if (NewState != DISABLE)
  {
    /* Enable the TIM Counter */
    TIMx->CR1 |= TIM_CR1_CEN;
  }
  else
  {
    /* Disable the TIM Counter */
    TIMx->CR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN));
  }
}

void TIM_OC1Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct)
{
  uint16_t tmpccmrx = 0, tmpccer = 0, tmpcr2 = 0;

 /* Disable the Channel 1: Reset the CC1E Bit */
  TIMx->CCER &= (uint16_t)(~(uint16_t)TIM_CCER_CC1E);
  /* Get the TIMx CCER register value */
  tmpccer = TIMx->CCER;
  /* Get the TIMx CR2 register value */
  tmpcr2 =  TIMx->CR2;

  /* Get the TIMx CCMR1 register value */
  tmpccmrx = TIMx->CCMR1;

  /* Reset the Output Compare Mode Bits */
  tmpccmrx &= (uint16_t)(~((uint16_t)TIM_CCMR1_OC1M));
  tmpccmrx &= (uint16_t)(~((uint16_t)TIM_CCMR1_CC1S));

  /* Select the Output Compare Mode */
  tmpccmrx |= TIM_OCInitStruct->TIM_OCMode;

  /* Reset the Output Polarity level */
  tmpccer &= (uint16_t)(~((uint16_t)TIM_CCER_CC1P));
  /* Set the Output Compare Polarity */
  tmpccer |= TIM_OCInitStruct->TIM_OCPolarity;

  /* Set the Output State */
  tmpccer |= TIM_OCInitStruct->TIM_OutputState;

  if((TIMx == TIM1) || (TIMx == TIM8)|| (TIMx == TIM15)||
     (TIMx == TIM16)|| (TIMx == TIM17))
  {

    /* Reset the Output N Polarity level */
    tmpccer &= (uint16_t)(~((uint16_t)TIM_CCER_CC1NP));
    /* Set the Output N Polarity */
    tmpccer |= TIM_OCInitStruct->TIM_OCNPolarity;

    /* Reset the Output N State */
    tmpccer &= (uint16_t)(~((uint16_t)TIM_CCER_CC1NE));
    /* Set the Output N State */
    tmpccer |= TIM_OCInitStruct->TIM_OutputNState;

    /* Reset the Output Compare and Output Compare N IDLE State */
    tmpcr2 &= (uint16_t)(~((uint16_t)TIM_CR2_OIS1));
    tmpcr2 &= (uint16_t)(~((uint16_t)TIM_CR2_OIS1N));

    /* Set the Output Idle state */
    tmpcr2 |= TIM_OCInitStruct->TIM_OCIdleState;
    /* Set the Output N Idle state */
    tmpcr2 |= TIM_OCInitStruct->TIM_OCNIdleState;
  }
  /* Write to TIMx CR2 */
  TIMx->CR2 = tmpcr2;

  /* Write to TIMx CCMR1 */
  TIMx->CCMR1 = tmpccmrx;

  /* Set the Capture Compare Register value */
  TIMx->CCR1 = TIM_OCInitStruct->TIM_Pulse;

  /* Write to TIMx CCER */
  TIMx->CCER = tmpccer;
}

void TIM_OC2Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct)
{
  uint16_t tmpccmrx = 0, tmpccer = 0, tmpcr2 = 0;

  /* Disable the Channel 2: Reset the CC2E Bit */
  TIMx->CCER &= (uint16_t)(~((uint16_t)TIM_CCER_CC2E));

  /* Get the TIMx CCER register value */
  tmpccer = TIMx->CCER;
  /* Get the TIMx CR2 register value */
  tmpcr2 =  TIMx->CR2;

  /* Get the TIMx CCMR1 register value */
  tmpccmrx = TIMx->CCMR1;

  /* Reset the Output Compare mode and Capture/Compare selection Bits */
  tmpccmrx &= (uint16_t)(~((uint16_t)TIM_CCMR1_OC2M));
  tmpccmrx &= (uint16_t)(~((uint16_t)TIM_CCMR1_CC2S));

  /* Select the Output Compare Mode */
  tmpccmrx |= (uint16_t)(TIM_OCInitStruct->TIM_OCMode << 8);

  /* Reset the Output Polarity level */
  tmpccer &= (uint16_t)(~((uint16_t)TIM_CCER_CC2P));
  /* Set the Output Compare Polarity */
  tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCPolarity << 4);

  /* Set the Output State */
  tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputState << 4);

  if((TIMx == TIM1) || (TIMx == TIM8))
  {

    /* Reset the Output N Polarity level */
    tmpccer &= (uint16_t)(~((uint16_t)TIM_CCER_CC2NP));
    /* Set the Output N Polarity */
    tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCNPolarity << 4);

    /* Reset the Output N State */
    tmpccer &= (uint16_t)(~((uint16_t)TIM_CCER_CC2NE));
    /* Set the Output N State */
    tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputNState << 4);

    /* Reset the Output Compare and Output Compare N IDLE State */
    tmpcr2 &= (uint16_t)(~((uint16_t)TIM_CR2_OIS2));
    tmpcr2 &= (uint16_t)(~((uint16_t)TIM_CR2_OIS2N));

    /* Set the Output Idle state */
    tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCIdleState << 2);
    /* Set the Output N Idle state */
    tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCNIdleState << 2);
  }
  /* Write to TIMx CR2 */
  TIMx->CR2 = tmpcr2;

  /* Write to TIMx CCMR1 */
  TIMx->CCMR1 = tmpccmrx;

  /* Set the Capture Compare Register value */
  TIMx->CCR2 = TIM_OCInitStruct->TIM_Pulse;

  /* Write to TIMx CCER */
  TIMx->CCER = tmpccer;
}


void TIM_OC3Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct)
{
  uint16_t tmpccmrx = 0, tmpccer = 0, tmpcr2 = 0;

  /* Disable the Channel 2: Reset the CC2E Bit */
  TIMx->CCER &= (uint16_t)(~((uint16_t)TIM_CCER_CC3E));

  /* Get the TIMx CCER register value */
  tmpccer = TIMx->CCER;
  /* Get the TIMx CR2 register value */
  tmpcr2 =  TIMx->CR2;

  /* Get the TIMx CCMR2 register value */
  tmpccmrx = TIMx->CCMR2;

  /* Reset the Output Compare mode and Capture/Compare selection Bits */
  tmpccmrx &= (uint16_t)(~((uint16_t)TIM_CCMR2_OC3M));
  tmpccmrx &= (uint16_t)(~((uint16_t)TIM_CCMR2_CC3S));
  /* Select the Output Compare Mode */
  tmpccmrx |= TIM_OCInitStruct->TIM_OCMode;

  /* Reset the Output Polarity level */
  tmpccer &= (uint16_t)(~((uint16_t)TIM_CCER_CC3P));
  /* Set the Output Compare Polarity */
  tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCPolarity << 8);

  /* Set the Output State */
  tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputState << 8);

  if((TIMx == TIM1) || (TIMx == TIM8))
  {

    /* Reset the Output N Polarity level */
    tmpccer &= (uint16_t)(~((uint16_t)TIM_CCER_CC3NP));
    /* Set the Output N Polarity */
    tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCNPolarity << 8);
    /* Reset the Output N State */
    tmpccer &= (uint16_t)(~((uint16_t)TIM_CCER_CC3NE));

    /* Set the Output N State */
    tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputNState << 8);
    /* Reset the Output Compare and Output Compare N IDLE State */
    tmpcr2 &= (uint16_t)(~((uint16_t)TIM_CR2_OIS3));
    tmpcr2 &= (uint16_t)(~((uint16_t)TIM_CR2_OIS3N));
    /* Set the Output Idle state */
    tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCIdleState << 4);
    /* Set the Output N Idle state */
    tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCNIdleState << 4);
  }
  /* Write to TIMx CR2 */
  TIMx->CR2 = tmpcr2;

  /* Write to TIMx CCMR2 */
  TIMx->CCMR2 = tmpccmrx;

  /* Set the Capture Compare Register value */
  TIMx->CCR3 = TIM_OCInitStruct->TIM_Pulse;

  /* Write to TIMx CCER */
  TIMx->CCER = tmpccer;
}

void TIM_OC4Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct)
{
  uint16_t tmpccmrx = 0, tmpccer = 0, tmpcr2 = 0;

  /* Disable the Channel 2: Reset the CC4E Bit */
  TIMx->CCER &= (uint16_t)(~((uint16_t)TIM_CCER_CC4E));

  /* Get the TIMx CCER register value */
  tmpccer = TIMx->CCER;
  /* Get the TIMx CR2 register value */
  tmpcr2 =  TIMx->CR2;

  /* Get the TIMx CCMR2 register value */
  tmpccmrx = TIMx->CCMR2;

  /* Reset the Output Compare mode and Capture/Compare selection Bits */
  tmpccmrx &= (uint16_t)(~((uint16_t)TIM_CCMR2_OC4M));
  tmpccmrx &= (uint16_t)(~((uint16_t)TIM_CCMR2_CC4S));

  /* Select the Output Compare Mode */
  tmpccmrx |= (uint16_t)(TIM_OCInitStruct->TIM_OCMode << 8);

  /* Reset the Output Polarity level */
  tmpccer &= (uint16_t)(~((uint16_t)TIM_CCER_CC4P));
  /* Set the Output Compare Polarity */
  tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OCPolarity << 12);

  /* Set the Output State */
  tmpccer |= (uint16_t)(TIM_OCInitStruct->TIM_OutputState << 12);

  if((TIMx == TIM1) || (TIMx == TIM8))
  {
    assert_param(IS_TIM_OCIDLE_STATE(TIM_OCInitStruct->TIM_OCIdleState));
    /* Reset the Output Compare IDLE State */
    tmpcr2 &= (uint16_t)(~((uint16_t)TIM_CR2_OIS4));
    /* Set the Output Idle state */
    tmpcr2 |= (uint16_t)(TIM_OCInitStruct->TIM_OCIdleState << 6);
  }
  /* Write to TIMx CR2 */
  TIMx->CR2 = tmpcr2;

  /* Write to TIMx CCMR2 */
  TIMx->CCMR2 = tmpccmrx;

  /* Set the Capture Compare Register value */
  TIMx->CCR4 = TIM_OCInitStruct->TIM_Pulse;

  /* Write to TIMx CCER */
  TIMx->CCER = tmpccer;
}

void TIM_OC1PreloadConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload)
{
  uint16_t tmpccmr1 = 0;

  tmpccmr1 = TIMx->CCMR1;
  /* Reset the OC1PE Bit */
  tmpccmr1 &= (uint16_t)~((uint16_t)TIM_CCMR1_OC1PE);
  /* Enable or Disable the Output Compare Preload feature */
  tmpccmr1 |= TIM_OCPreload;
  /* Write to TIMx CCMR1 register */
  TIMx->CCMR1 = tmpccmr1;
}

void TIM_OC2PreloadConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload)
{
  uint16_t tmpccmr1 = 0;

  tmpccmr1 = TIMx->CCMR1;
  /* Reset the OC2PE Bit */
  tmpccmr1 &= (uint16_t)~((uint16_t)TIM_CCMR1_OC2PE);
  /* Enable or Disable the Output Compare Preload feature */
  tmpccmr1 |= (uint16_t)(TIM_OCPreload << 8);
  /* Write to TIMx CCMR1 register */
  TIMx->CCMR1 = tmpccmr1;
}

void TIM_OC3PreloadConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload)
{
  uint16_t tmpccmr2 = 0;

  tmpccmr2 = TIMx->CCMR2;
  /* Reset the OC3PE Bit */
  tmpccmr2 &= (uint16_t)~((uint16_t)TIM_CCMR2_OC3PE);
  /* Enable or Disable the Output Compare Preload feature */
  tmpccmr2 |= TIM_OCPreload;
  /* Write to TIMx CCMR2 register */
  TIMx->CCMR2 = tmpccmr2;
}

void TIM_OC4PreloadConfig(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload)
{
  uint16_t tmpccmr2 = 0;

  tmpccmr2 = TIMx->CCMR2;
  /* Reset the OC4PE Bit */
  tmpccmr2 &= (uint16_t)~((uint16_t)TIM_CCMR2_OC4PE);
  /* Enable or Disable the Output Compare Preload feature */
  tmpccmr2 |= (uint16_t)(TIM_OCPreload << 8);
  /* Write to TIMx CCMR2 register */
  TIMx->CCMR2 = tmpccmr2;
}

void ChangeMotorPwm (uint16_t pwm1, uint16_t pwm2, uint8_t mask)
{
  if (mask & 1) {
    TIM4->CCR1 = pwm1;
    TIM4->CCR2 = 0;
  } else if (mask & 2) {
    TIM4->CCR1 = 0;
    TIM4->CCR2 = pwm1;
  } else if (mask & 0x30) {
    TIM4->CCR1 = pwm1;
    TIM4->CCR2 = pwm1;
  } else {
    TIM4->CCR1 = 0;
    TIM4->CCR2 = 0;
  }

  if (mask & 4) {
    TIM4->CCR3 = pwm2;
    TIM4->CCR4 = 0;
  } else if (mask & 8) {
    TIM4->CCR3 = 0;
    TIM4->CCR4 = pwm2;
  } else if (mask & 0xC0) {
    TIM4->CCR3 = pwm2;
    TIM4->CCR4 = pwm2;
  } else {
    TIM4->CCR3 = 0;
    TIM4->CCR4 = 0;
  }
}

void MotorInit (BYTE Pulse)
{
  GPIO_InitTypeDef         GPIO_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_OCInitTypeDef        TIM_OCInitStructure;

  /* Timer OC1 configuration */
  TIM_OCInitStructure.TIM_Pulse        = Pulse;
  TIM_OCInitStructure.TIM_OCMode       = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OCPolarity   = TIM_OCPolarity_High;
  TIM_OCInitStructure.TIM_OutputState  = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_OCNPolarity  = TIM_OCNPolarity_Low;   // NA
  TIM_OCInitStructure.TIM_OCIdleState  = TIM_OCIdleState_Set;   // NA
  TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset; // NA
  TIM_OC1Init(TIM4, &TIM_OCInitStructure);
  TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
  TIM_OC2Init(TIM4, &TIM_OCInitStructure);
  TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);
  TIM_OC3Init(TIM4, &TIM_OCInitStructure);
  TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);
  TIM_OC4Init(TIM4, &TIM_OCInitStructure);
  TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Prescaler = 71;                        //  72M/(x+1) = 1M
  TIM_TimeBaseStructure.TIM_Period    = 200;                       //  5KHz
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0x0;
  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

  /* Enable PB6-PB9 */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  gpioinit (GPIOB, &GPIO_InitStructure);

  /* Set duty */
  ChangeMotorPwm (Pulse, Pulse, 0x0F);

  /* Enable counter */
  TIM4->CNT = 0;
  TIM4->SR  = 0;
  TIM_Cmd(TIM4, ENABLE);
}


#define SERVO_PWR_ON     GPIO_SetBits(GPIOB,   GPIO_Pin_4)
#define SERVO_PWR_OFF    GPIO_ResetBits(GPIOB, GPIO_Pin_4)

#define PWM_OE_ON        GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define PWM_OE_OFF       GPIO_SetBits(GPIOA, GPIO_Pin_4)


#define  CHN_NUM   6

const BYTE PwmChTbl[CHN_NUM] = {0,1,2,3,4,5};

#define PWM_MIN  0x040
#define PWM_MAX  0x180
const BYTE PwmScopeTbl[CHN_NUM * 2] =
{
   0xB0 - PWM_MIN, 0x1E0 - PWM_MAX, // 0 (L Hand: 0x1E0 UP)
   0xB0 - PWM_MIN, 0x1E0 - PWM_MAX, // 1 (R Hand: 0xB0  UP)
   0xF8 - PWM_MIN, 0x188 - PWM_MAX, // 2 (  Head: 0x148 Mid)
   0xB0 - PWM_MIN, 0x1E0 - PWM_MAX, // 3 (Unused)
   0x00, 0x00, // 5 (L eye)
   0x00, 0x00, // 5 (R eye)
};

WORD PwmInitTbl[CHN_NUM] =
{
   0x1C0,
   0xCC,
   0x132,
   0x00,
   0x00,
   0x00
};

void RCC_APB1PeriphClockCmd(uint32_t RCC_APB1Periph, FunctionalState NewState)
{
  assert_param(IS_RCC_APB1_PERIPH(RCC_APB1Periph));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {
    RCC->APB1ENR |= RCC_APB1Periph;
  }
  else
  {
    RCC->APB1ENR &= ~RCC_APB1Periph;
  }
}

void RCC_APB1PeriphResetCmd(uint32_t RCC_APB1Periph, FunctionalState NewState)
{
  assert_param(IS_RCC_APB1_PERIPH(RCC_APB1Periph));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {
    RCC->APB1RSTR |= RCC_APB1Periph;
  }
  else
  {
    RCC->APB1RSTR &= ~RCC_APB1Periph;
  }
}

void RCC_APB2PeriphResetCmd(uint32_t RCC_APB2Periph, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_RCC_APB2_PERIPH(RCC_APB2Periph));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {
    RCC->APB2RSTR |= RCC_APB2Periph;
  }
  else
  {
    RCC->APB2RSTR &= ~RCC_APB2Periph;
  }
}

void RCC_ADCCLKConfig(uint32_t RCC_PCLK2)
{
  #define CFGR_ADCPRE_Reset_Mask    ((uint32_t)0xFFFF3FFF)

  uint32_t tmpreg = 0;
  /* Check the parameters */
  assert_param(IS_RCC_ADCCLK(RCC_PCLK2));
  tmpreg = RCC->CFGR;
  /* Clear ADCPRE[1:0] bits */
  tmpreg &= CFGR_ADCPRE_Reset_Mask;
  /* Set ADCPRE[1:0] bits according to RCC_PCLK2 value */
  tmpreg |= RCC_PCLK2;
  /* Store the new value */
  RCC->CFGR = tmpreg;
}

void RCC_APB2PeriphClockCmd(uint32_t RCC_APB2Periph, FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_RCC_APB2_PERIPH(RCC_APB2Periph));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {
    RCC->APB2ENR |= RCC_APB2Periph;
  }
  else
  {
    RCC->APB2ENR &= ~RCC_APB2Periph;
  }
}

void RCC_GetClocksFreq(RCC_ClocksTypeDef* RCC_Clocks)
{
  RCC_Clocks->PCLK1_Frequency = 0x2255100;
}

#include "stm32f10x_i2c.c"

#define PCA9685_ADDR  0x80

BYTE Pca9686Start (BYTE Offset)
{
  WORD Timeout;

  //warte auf I2C
  while(I2C_GetFlagStatus(I2C2,I2C_FLAG_BUSY));

  //Start
  I2C_GenerateSTART(I2C2, ENABLE);

  Timeout = 0xFF;
  while(Timeout && !I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)) {
    Timeout--;
  }
  if (Timeout == 0) {
    return 1;
  }

  //Address
  I2C_Send7bitAddress (I2C2, PCA9685_ADDR, I2C_Direction_Transmitter);

  Timeout = 0xFF;
  while(Timeout && !I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
    Timeout--;
  }
  if (Timeout == 0) {
    return 1;
  }

  //Register offset
  I2C_SendData(I2C2, Offset);

  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  return 0;
}

BYTE Pca9685Write(BYTE Offset, BYTE Len, BYTE *Buf)
{
  BYTE Ret;

  Ret = Pca9686Start (Offset);

  if (Ret == 0) {
    //Send data
    while (Len--) {
      I2C_SendData(I2C2, *Buf++);
      while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    }
  }

  I2C_GenerateSTOP(I2C2,ENABLE); // Send STOP condition
  return Ret;
}

BYTE Pca9685Read(BYTE Offset)
{
  BYTE Value;

  Pca9686Start (Offset);

  // Send repeated START condition (aka Re-START)
  I2C_GenerateSTART(I2C2, ENABLE);
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));

  //Address
  I2C_Send7bitAddress (I2C2, PCA9685_ADDR, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(I2C2,I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

  // Wait for data
  while (!I2C_CheckEvent(I2C2,I2C_EVENT_MASTER_BYTE_RECEIVED));

  // Read data
  Value = I2C_ReceiveData(I2C2);

	I2C_AcknowledgeConfig(I2C2, DISABLE); // Disable I2C acknowledgment
	I2C_GenerateSTOP(I2C2,ENABLE); // Send STOP condition

  return Value;
}


WORD ReadPwm (BYTE Ch)
{
  WORD  Val;
  BYTE *Ptr;

  Ch = PwmChTbl[Ch];
  Ptr = (BYTE *)&Val;
  Ptr[0] = Pca9685Read (0x08 + (Ch << 2));
  Ptr[1] = Pca9685Read (0x09 + (Ch << 2));

  return Val;
}

void ChangeServoPwm (BYTE Ch, WORD Off)
{
  BYTE     Buf[4];

  Ch = PwmChTbl[Ch];
  Buf[0] = 0x00 & 0xFF;
  Buf[1] = 0x00 >> 8;
  Buf[2] = Off & 0xFF;
  Buf[3] = Off >> 8;
  Pca9685Write (0x06 + Ch * 4,  4, Buf);
}

WORD AdjustPwmValue (UINT8 PwmCh, WORD PwmVal)
{
  WORD  Min,Max;

  Min = (WORD)PwmScopeTbl[(PwmCh<<1)+0] + PWM_MIN;
  Max = (WORD)PwmScopeTbl[(PwmCh<<1)+1] + PWM_MAX;
  if (Min == PWM_MIN) {
    Min = 0;
  }
  if (Max == PWM_MAX) {
    Max = 0xFFF;
  }
  if (PwmVal > Max) {
    PwmVal = Max;
  }
  if (PwmVal < Min) {
    PwmVal = Min;
  }
  return PwmVal;
}

BYTE  gPwmCh;
WORD  gPwmVal;
void Ctrl (BYTE Ch)
{
  WORD  Min,Max;
  BYTE  Step;

  if (Ch >= '0' && Ch < '0' + CHN_NUM) {
    gPwmCh = Ch - '0';
    gPwmVal = ReadPwm(gPwmCh);
    printf ("R: %2d %04X\n", gPwmCh, gPwmVal);
  } else if ((Ch == '+') || (Ch == '-')) {
    Step = gPwmCh > 3 ? 0x80 : 0x01 ;
    if (Ch == '+') {
      gPwmVal += Step;
    } else {
      gPwmVal -= Step;
    }
    Min = (WORD)PwmScopeTbl[(gPwmCh<<1)+0] + PWM_MIN;
    Max = (WORD)PwmScopeTbl[(gPwmCh<<1)+1] + PWM_MAX;
    if (Min == PWM_MIN) {
      Min = 0;
    }
    if (Max == PWM_MAX) {
      Max = 0xFFF;
    }
    if (gPwmVal > Max) {
      gPwmVal = Max;
    }
    if (gPwmVal < Min) {
      gPwmVal = Min;
    }
    printf ("W: %2d %04X\n", gPwmCh, gPwmVal);
    ChangeServoPwm (gPwmCh, gPwmVal);
  }
}

BYTE Pca9685Init ()
{
	I2C_InitTypeDef          I2C_InitStruct;
  GPIO_InitTypeDef         GPIO_InitStructure;
  BYTE                     Buf[1];
  BYTE                     Idx;

	// Step 1: Initialize I2C
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
	I2C_InitStruct.I2C_ClockSpeed = 1000000;
	I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStruct.I2C_OwnAddress1 = 0x00;
	I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

  I2C_Init(I2C2, &I2C_InitStruct);
  I2C_Cmd(I2C2, ENABLE);

  /* Enable PA4 (PWM OE) & PB4 (Servo PWR) as output */
  SERVO_PWR_OFF;
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  gpioinit(GPIOA, &GPIO_InitStructure);
  gpioinit(GPIOB, &GPIO_InitStructure);

  /* Enable PB10/PB11 as I2C */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_OD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  gpioinit(GPIOB, &GPIO_InitStructure);

  PWM_OE_ON;

  /* 25M / (4096 * Hz) - 1,  60hz */
  Buf[0] = 0x10;
  if (Pca9685Write (0x00, 1, Buf)) {
    return 1;
  }

  Buf[0] = 0x79;
  Pca9685Write (0xFE, 1, Buf);

  Buf[0] = 0xA1;
  Pca9685Write (0x00, 1, Buf);

  for (Idx = 0; Idx < CHN_NUM; Idx++) {
    ChangeServoPwm (Idx, PwmInitTbl[Idx]);
  }

  SERVO_PWR_ON;

  if (Debug) {
    printf ("SERVO WPM = ");
    for (Idx = 0; Idx < CHN_NUM; Idx++) {
      printf ("%d:%04X ", Idx, ReadPwm (Idx));
    }
    printf ("\n");
  }

  return 0;
}

#define  CS_LOW       GPIO_ResetBits(GPIOB, GPIO_Pin_12)
#define  CS_HIGH      GPIO_SetBits(GPIOB,   GPIO_Pin_12)
#define  CSN(x)       if (x) CS_HIGH; else CS_LOW
#define  CE_LOW       GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#define  CE_HIGH      GPIO_SetBits(GPIOB,   GPIO_Pin_0)
#define  CE(x)        if (x) CE_HIGH; else CE_LOW
#define  SpiWrite     spiwrite
#define  DelayUs      delayUS_ASM

#include "Inc/Rf24L01.c"


const BYTE PollState[] = {
  0x01,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

void Ps2Init ()
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


// It is important to keep the byte and frame delay
void PreDelay (WORD ms)
{
  volatile WORD i;
  for(; ms>0; ms--) for (i = 0; i < 1; i++);
}


#define DELAY_BYTE_US   10
#define DELAY_FRAME_MS  18
void Ps2SendFrame (const BYTE *Cmd, BYTE Len, BYTE *Buf)
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


#undef   CR1_CLEAR_Mask
#include "stm32f10x_adc.c"
void BatAdcInit ()
{
  GPIO_InitTypeDef   GPIO_InitStructure;
  ADC_InitTypeDef    ADC_InitStructure;

  /* Enable PB1 as ADC */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AIN;
  gpioinit(GPIOB, &GPIO_InitStructure);


  /* PCLK2 is the APB2 clock */
  /* ADCCLK = PCLK2/6 = 72/6 = 12MHz*/
  RCC_ADCCLKConfig(RCC_PCLK2_Div6);

  /* Enable ADC1 clock so that we can talk to it */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
  /* Put everything back to power-on defaults */
  ADC_DeInit(ADC1);

  /* ADC1 Configuration ------------------------------------------------------*/
  /* ADC1 and ADC2 operate independently */
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  /* Disable the scan conversion so we do one at a time */
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  /* Don't do contimuous conversions - do them on demand */
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  /* Start conversin by software, not an external trigger */
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  /* Conversions are 12 bit - put them in the lower 12 bits of the result */
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  /* Say how many channels would be used by the sequencer */
  ADC_InitStructure.ADC_NbrOfChannel = 1;

  /* Now do the setup */
  ADC_Init(ADC1, &ADC_InitStructure);
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);

  /* Enable ADC1 reset calibaration register */
  ADC_ResetCalibration(ADC1);
  /* Check the end of ADC1 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));
  /* Start ADC1 calibaration */
  ADC_StartCalibration(ADC1);
  /* Check the end of ADC1 calibration */
  while(ADC_GetCalibrationStatus(ADC1));
}


WORD ReadBatAdc ()
{
  BYTE  Channel;

  Channel = 9;
  ADC_RegularChannelConfig(ADC1, Channel, 1, ADC_SampleTime_1Cycles5);
  // Start the conversion
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
  // Wait until conversion completion
  while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
  // Get the conversion value
  return ADC_GetConversionValue(ADC1);
}

#define   SONIC_PORT     B
#define   SONIC_PIN      3
#define   SONIC_IRQ_NUM  I3
#define   SONIC_IRQ             STR_CAT3(EXT, SONIC_IRQ_NUM, _IRQn)
#define   SONIC_IRQ_VECT        Vect_IRQHandler(SONIC_IRQ)

#define   TRIGGER_ON            GPIO_SetBits(GPIOB,   GPIO_Pin_5)
#define   TRIGGER_OFF           GPIO_ResetBits(GPIOB, GPIO_Pin_5)

DefExtiInit(SONIC);

volatile WORD  Distance;

void ExtIrq_SONIC (void)
{
  static DWORD  TsTick;
  DWORD  Ts;

  Ts = GetSysTick();

  if (GET_GPIO (SONIC)) {
    // Rising
    TsTick = Ts;
  } else {
    // Falling
    TsTick = Ts - TsTick;
    Distance = (WORD)TsTick;
    DisableExtInterrupt  (SONIC);
  }
  EXTI->PR   |=  GPIO_PIN(SONIC_PIN);
}

void SonicInit ()
{
  GPIO_InitTypeDef         GPIO_InitStructure;

  /* Enable PB3:echo and PB5:trigger as output */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  gpioinit(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  gpioinit(GPIOB, &GPIO_InitStructure);
  TRIGGER_OFF;

  DisableExtInterrupt (SONIC);
  SET_ISR  (SONIC_IRQ_VECT,  ExtIrq_SONIC);

  ExtiInit_SONIC (0, 0);

  EXTI->FTSR  |=  GPIO_PIN(SONIC_PIN);

}

void BoardInit ()
{
  GPIO_InitTypeDef         GPIO_InitStructure;

  /* PB2 (BOOT1) as input for TOP button */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  gpioinit(GPIOB, &GPIO_InitStructure);

  /* PC13 (LED) as output, low ON */
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  gpioinit(GPIOC, &GPIO_InitStructure);

  LED_A_OFF;
}

void SysTickIrq(void)
{
  // around 50ms
  SysTickCnt++;

  // trigger distance measure
  TRIGGER_ON;
  delayUS_ASM (10);
  TRIGGER_OFF;
  EnableExtInterrupt (SONIC);
}

void SysTickInit(void)
{
  SET_ISR  (Vect_SysTick_Handler,  SysTickIrq);
  SysTick_Config (0xFFFFF);
}

void ErrorCode (BYTE Error)
{
  BYTE  Idx;

  while (1) {
    for (Idx = 0; Idx < 8; Idx++) {
      if (Error & (1<<Idx)) {
        LED_A_ON;
      } else {
        LED_A_OFF;
      }
      DelayMs (300);
    }
  }
}


#define  MOTOR_PWR  0x80

WORD     BatVol;
BYTE     Motor[2];
BYTE     MotorState;

BYTE ProcessPacket (BYTE *RxBuf, BYTE *AckBuf)
{
  BYTE  Len;

  Len = TX_DATA_WITDH;

  AckBuf[0] = Motor[0];
  AckBuf[1] = Motor[1];
  AckBuf[2] = MotorState;
  *(WORD *)&AckBuf[4] = BatVol;
  *(WORD *)&AckBuf[6] = Distance;
  memcpy (&AckBuf[8], (void *)PwmScopeTbl, CHN_NUM * 2);

  return Len;
}

#define  USE_RF24   0
#define  USE_PS2    1

void Walle ()
{
  DWORD    Tick;
  UINT32   LastTickCnt;
  BYTE     Inp;
  BYTE     Idx;
  BYTE     Ch;
  BYTE     Pca;
  BYTE     MotorRun;
  BYTE     Loop;
  BYTE     Data[TX_DATA_WITDH];
  WORD     Servo[3];
  BYTE     Demo;

  if (IsUsbCablePlugged()) {
    PrtOn = 1;
  } else {
    PrtOn = 0;
  }

  AFIO->EXTICR[0] = 0;

  SysTickInit ();
  BoardInit ();

  SonicInit ();
  MotorInit (MG996R_MIN);
  BatAdcInit();
  BatVol = ReadBatAdc() * 49 / 10;
  if (!PrtOn && (BatVol < 6000)) {
    ErrorCode (0xFE);
  }

  printf ("BatV: %d mV\n", BatVol);
  // BatVol  8780

#if USE_RF24
  Rf2400Init(SPI2);
  printf ("RF24: %s\n", NRFReadReg (0x03) == 0x03 ? "OK" : "Not present");
  NRFSetRxMode ();
#endif

#if USE_PS2
  printf ("Use PS2\n");
  Ps2Init ();
#endif

  Pca = 1 - Pca9685Init ();
  printf ("9685: %s\n", Pca ? "OK" : "Not present");

  // 0xAC -> 0xA8
  // 0xAC:6.12v 0xB0:6.50v B4: 6.88  0xB8: 7.28  0xBC: 7.66 0xC0 8v  0xC8 8.8v
  // ChangeMotorPwm (0x00, 0x00, 0x05);
  if (BatVol >= 6000) {
    Motor[0] = Motor[1] = (BYTE)(144 - ((BatVol - 6000) >> 8));
  } else {
    Motor[0] = Motor[1] = MOTOR_PWR;
  }

  Servo[0] = PwmInitTbl[0];
  Servo[1] = PwmInitTbl[1];
  Servo[2] = PwmInitTbl[2];

  Demo  = 0;
  Tick  = 0;
  Loop  = 1;
  MotorRun    = 0;
  LastTickCnt = SysTickCnt;
  while (Loop) {

    if (!PrtOn && IsUsbCablePlugged()) {
      PrtOn = 1;
    } else if (PrtOn && !IsUsbCablePlugged()) {
      PrtOn = 0;
    }

    Inp  = 0x00;

#if USE_RF24
    if (NRFRxPacket (Data) == 1) {
      if (Data[0] == '$') {
        Inp = Data[1];
      } else if (Data[0] == '@') {
        switch (Data[1]) {
        case 'T':
          // Set motor
          if (Data[2] & 1) {
            ChangeMotorPwm (Data[4], Data[5], Data[6]);
          }
          // Set servo
          for (Idx = 0; Idx < 8; Idx++) {
            if (Data[3] & (1 << Idx)) {
              ChangeServoPwm (Idx, *(WORD *)&Data[8+(Idx<<1)]);
            }
          }
          break;
        }
        Inp = 0;
      }
    } else
#endif

    if (haschar()) {
      Inp = getchar();
      if (Inp == 0x1b) {
        Loop = 0;
      }
    } else {
      Inp = 0;
    }

    if (Demo) {
      Inp = 0;
      // LT ARM  (0, 0 - 0xFFF);
      // RT ARM  (1, 0 - 0xFFF);
      // CT HEAD (2, 0 - 0xFFF);
      // LT EYE  (4, 0 - 0xFFF);
      // RT EYE  (5, 0 - 0xFFF);
      // Move
      //   Forward 5
      //   Right   6
      //   Back    A
      //   Left    9

      // Wave hand
      ChangeMotorPwm (Motor[0], Motor[1], 0x05);
      ChangeServoPwm (2, 0x188);
      ChangeServoPwm (4, 0xFFF);
      ChangeServoPwm (5, 0xFFF);
      for (Idx = 0; Idx < 4; Idx++) {
        ChangeServoPwm (1, 0x1B0);
        DelayMs (400);
        ChangeServoPwm (1, 0xCC);
        DelayMs (400);
      }

      // Dance
      for (Idx = 0; Idx < 2; Idx++) {
        ChangeServoPwm (0, 0xB0);  // ARM L U- D+ (B0-1E0)
        ChangeServoPwm (1, 0x100); // ARM R U+ D- (1E0-B0)

        ChangeServoPwm (2, 0xF8);  // Head
        ChangeServoPwm (4, 0);     // Eye
        ChangeServoPwm (5, 0xFFF); // Eye

        ChangeMotorPwm (Motor[0], Motor[1], 0x06);
        DelayMs (600);

        ChangeServoPwm (0, 0x100); // ARM L U- D+ (B0-1E0)
        ChangeServoPwm (1, 0x1E0); // ARM R U+ D- (1E0-B0)
        ChangeServoPwm (2, 0x188);
        ChangeServoPwm (4, 0xFFF);
        ChangeServoPwm (5, 0);
        ChangeMotorPwm (Motor[0], Motor[1], 0x09);
        DelayMs (600);
      }

      ChangeMotorPwm (0, 0, 5);
      for (Idx = 0; Idx < 5; Idx++) {
        ChangeServoPwm (Idx, PwmInitTbl[Idx]);
      }
      Demo = 0;
    }

    if (Inp != 0) {
      if (Inp == '!') {
        PrtOn = 0;
      } else if (Inp == '@') {
        PrtOn = 1;
      } else if (Inp == 'b') {
        MotorState = 0x0A;
        ChangeMotorPwm (Motor[0], Motor[1], 0x0A);
      } else if (Inp == 'f') {
        MotorState = 0x05;
        ChangeMotorPwm (Motor[0], Motor[1], 0x05);
      } else if (Inp == 'r') {
        MotorState = 0x0A;
        ChangeMotorPwm (Motor[0], Motor[1], 0x06);
      } else if (Inp == 'l') {
        ChangeMotorPwm (Motor[0], Motor[1], 0x09);
      } else if (Inp == 'h') {
        ChangeMotorPwm (0x00, 0x00, 0x05);
      } else if (Inp == 'v') {
        BatAdcInit();
        BatVol = ReadBatAdc() * 49 / 10;
        printf ("Voltage: %d\n", BatVol);
      } else if (Inp == 'd') {
        printf ("Distance: %d\n", Distance);
      } else if (Pca) {
        BYTE  Step;

        Ch = Inp;
        if (Ch >= '0' && Ch <= '9') {
          gPwmCh = Ch - '0';
          if (gPwmCh < CHN_NUM) {
            gPwmVal = ReadPwm(gPwmCh);
            printf ("R: %2d %04X\n", gPwmCh, gPwmVal);
          } else {
            if (gPwmCh >= 8) {
              gPwmVal = Motor[gPwmCh - 8];
              printf ("R: %2d %02X\n", gPwmCh, Motor[gPwmCh - 8]);
            }
          }
        } else if ((Ch == '=') || (Ch == '-')) {
          if (gPwmCh >= 8) {
            Step = 8;
          } else if (gPwmCh > 3) {
            Step = 0x80;
          } else {
            Step = 1;
          }
          if (Ch == '=') {
            gPwmVal += Step;
          } else {
            gPwmVal -= Step;
          }

          if (gPwmCh < CHN_NUM) {
            gPwmVal = AdjustPwmValue (gPwmCh, gPwmVal);
            printf ("W: %2d %04X\n", gPwmCh, gPwmVal);
            ChangeServoPwm (gPwmCh, gPwmVal);
          } else {
             if (gPwmVal > 0xC0) {
              gPwmVal = 0xC0;
            }
            if (gPwmVal < 0x40) {
              gPwmVal = 0x40;
            }
            Motor[gPwmCh - 8] = gPwmVal;
            printf ("W: %2d %04X\n", gPwmCh, gPwmVal);
          }
        }
      }
    }

#if USE_PS2
    if (SysTickCnt < LastTickCnt) {
      LastTickCnt = SysTickCnt;
    } else if (SysTickCnt - LastTickCnt > 10) {
      Ps2SendFrame (PollState, 5, Data);
      LastTickCnt = SysTickCnt;
      if (0) {
        for (Idx = 0; Idx < 5; Idx++) {
          printf ("0x%02X ", Data[Idx]);
        }
        printf ("\n");
      }

      if (Data[2] == 0x5A) {
        if ((Data[3] & 0x01) == 0x00) {
          Demo = 1;
        }

        if ((Data[4] & 0x0F) != 0x0F) {
          if ((Data[4] & 0x04) == 0) {
            ChangeServoPwm (5, 0xFFF);
          } else if ((Data[4] & 0x08) == 0) {
            ChangeServoPwm (4, 0xFFF);
          } else if ((Data[4] & 0x02) == 0) {
            ChangeServoPwm (4, 0);
          } else if ((Data[4] & 0x01) == 0) {
            ChangeServoPwm (5, 0);
          }
        }

        if ((Data[3] & 0xF0) != 0xF0) {
          if ((Data[3] & 0x10) == 0) {
            // Forward
            Servo[0] = AdjustPwmValue (0, Servo[0] + 0x08);
            Servo[1] = AdjustPwmValue (1, Servo[1] - 0x08);
            if ((Data[3] & 0x08) == 0x08) {
              ChangeServoPwm (0, Servo[0]);
            }
            if ((Data[3] & 0x01) == 0x01) {
              ChangeServoPwm (1, Servo[1]);
            }
          } else if ((Data[3] & 0x20) == 0) {
            // Right
            Servo[2] = AdjustPwmValue (2, Servo[2] - 0x08);
            ChangeServoPwm (2, Servo[2]);
          } else if ((Data[3] & 0x40) == 0) {
            // Back
            Servo[0] = AdjustPwmValue (0, Servo[0] - 0x08);
            Servo[1] = AdjustPwmValue (1, Servo[1] + 0x08);
            if ((Data[3] & 0x08) == 0x08) {
              ChangeServoPwm (0, Servo[0]);
            }
            if ((Data[3] & 0x01) == 0x01) {
              ChangeServoPwm (1, Servo[1]);
            }
          } else if ((Data[3] & 0x80) == 0) {
            // Left
            Servo[2] = AdjustPwmValue (2, Servo[2] + 0x08);
            ChangeServoPwm (2, Servo[2]);
          }
        }

        if ((Data[4] & 0xF0) != 0xF0) {
          if ((Data[4] & 0x10) == 0) {
            // Forward
            MotorState = 0x05;
          } else if ((Data[4] & 0x20) == 0) {
            // Right
            MotorState = 0x06;
          } else if ((Data[4] & 0x40) == 0) {
            // Back
            MotorState = 0x0A;
          } else if ((Data[4] & 0x80) == 0) {
            // Left
            MotorState = 0x09;
          }
          MotorRun = 1;
          ChangeMotorPwm (Motor[0], Motor[1], MotorState);
        } else {
          if (MotorRun) {
            ChangeMotorPwm (0, 0, 5);
            MotorRun = 0;
          }
        }
      }
    }
#endif

    if (Tick & 0x2000000) {
      LED_A_ON;
    } else {
      LED_A_OFF;
    }

    Tick = GetSysTick ();
  }

}
