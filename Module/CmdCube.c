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
    {'C', 'U', 'B', 'E'},
    { 0,  0,  0,  0 },
    { 0,  0,  0,  0 },
    { 0,  0,  0,  0 },
  }
};

COMMON_API *gCommonApi = (COMMON_API *)COMMON_API_BASE;

const char * const gCmdHelper[CMD_NUM] = {
  "                      ;Rubik's Cube",
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

void DelayMs (WORD ms)
{
  volatile WORD i, j;
  for (; ms > 0; ms--)
    for (i = 0; i < 685; i++)
      for (j = 0; j < 5; j++);
}

void DelayUs (WORD us)
{
  volatile WORD j;
  for (; us > 0; us--)
      for (j = 0; j < 10; j++) __NOP();
}

void GPIO_ResetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
  GPIOx->BRR = GPIO_Pin;
}

#define EVCR_PORTPINCONFIG_MASK     ((uint16_t)0xFF80)
#define LSB_MASK                    ((uint16_t)0xFFFF)
#define DBGAFR_POSITION_MASK        ((uint32_t)0x000F0000)
#define DBGAFR_SWJCFG_MASK          ((uint32_t)0xF0FFFFFF)
#define DBGAFR_LOCATION_MASK        ((uint32_t)0x00200000)
#define DBGAFR_NUMBITS_MASK         ((uint32_t)0x00100000)

void GPIO_PinRemapConfig(uint32_t GPIO_Remap, FunctionalState NewState)
{
  uint32_t tmp = 0x00, tmp1 = 0x00, tmpreg = 0x00, tmpmask = 0x00;

  if((GPIO_Remap & 0x80000000) == 0x80000000)
  {
    tmpreg = AFIO->MAPR2;
  }
  else
  {
    tmpreg = AFIO->MAPR;
  }

  tmpmask = (GPIO_Remap & DBGAFR_POSITION_MASK) >> 0x10;
  tmp = GPIO_Remap & LSB_MASK;

  if ((GPIO_Remap & (DBGAFR_LOCATION_MASK | DBGAFR_NUMBITS_MASK)) == (DBGAFR_LOCATION_MASK | DBGAFR_NUMBITS_MASK))
  {
    tmpreg &= DBGAFR_SWJCFG_MASK;
    AFIO->MAPR &= DBGAFR_SWJCFG_MASK;
  }
  else if ((GPIO_Remap & DBGAFR_NUMBITS_MASK) == DBGAFR_NUMBITS_MASK)
  {
    tmp1 = ((uint32_t)0x03) << tmpmask;
    tmpreg &= ~tmp1;
    tmpreg |= ~DBGAFR_SWJCFG_MASK;
  }
  else
  {
    tmpreg &= ~(tmp << ((GPIO_Remap >> 0x15)*0x10));
    tmpreg |= ~DBGAFR_SWJCFG_MASK;
  }

  if (NewState != DISABLE)
  {
    tmpreg |= (tmp << ((GPIO_Remap >> 0x15)*0x10));
  }

  if((GPIO_Remap & 0x80000000) == 0x80000000)
  {
    AFIO->MAPR2 = tmpreg;
  }
  else
  {
    AFIO->MAPR = tmpreg;
  }
}


void RCC_APB2PeriphClockCmd(uint32_t RCC_APB2Periph, FunctionalState NewState)
{
  /* Check the parameters */
  if (NewState != DISABLE)
  {
    RCC->APB2ENR |= RCC_APB2Periph;
  }
  else
  {
    RCC->APB2ENR &= ~RCC_APB2Periph;
  }
}

void RCC_APB1PeriphClockCmd(uint32_t RCC_APB1Periph, FunctionalState NewState)
{
  if (NewState != DISABLE)
  {
    RCC->APB1ENR |= RCC_APB1Periph;
  }
  else
  {
    RCC->APB1ENR &= ~RCC_APB1Periph;
  }
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

void pwm_init ()
{
  TIM_TimeBaseInitTypeDef  TimerInitStructure;
  TIM_OCInitTypeDef        OutputChannelInit;
  GPIO_InitTypeDef         GPIO_InitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  gpioinit (GPIOA, &GPIO_InitStructure);

  TimerInitStructure.TIM_Prescaler = 72 - 1;
  TimerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TimerInitStructure.TIM_Period = 20000;
  TimerInitStructure.TIM_ClockDivision = 0;
  TimerInitStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(TIM2, &TimerInitStructure);

  OutputChannelInit.TIM_OCMode = TIM_OCMode_PWM1;
  OutputChannelInit.TIM_Pulse  = SERVO_OPEN_VAL;
  OutputChannelInit.TIM_OutputState = TIM_OutputState_Enable;
  OutputChannelInit.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OC1Init(TIM2, &OutputChannelInit);
  TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);

  /* Enable counter */
  TIM2->CNT = 0;
  TIM2->SR  = 0;
  TIM_Cmd(TIM2, ENABLE);
}


#define LED_A_ON         GpioSetBits(GPIOC, GPIO_Pin_13)
#define LED_A_OFF        GpioClrBits(GPIOC, GPIO_Pin_13)
#define LED_A_STS        GpioGetBits(GPIOC, GPIO_Pin_13)

#define  STEP_F_FW()        GpioClrBits(GPIOB, GPIO_Pin_13)
#define  STEP_F_BK()        GpioSetBits(GPIOB, GPIO_Pin_13)

#define  STEP_SL_FW()       GpioClrBits(GPIOB, GPIO_Pin_11)
#define  STEP_SL_BK()       GpioSetBits(GPIOB, GPIO_Pin_11)

#define  STEP_SR_FW()       GpioClrBits(GPIOB, GPIO_Pin_12)
#define  STEP_SR_BK()       GpioSetBits(GPIOB, GPIO_Pin_12)
#define  STEP_SR_PULSE_PIN  GPIO_Pin_14

#define  STEP_ON()          GpioClrBits(GPIOA, GPIO_Pin_1)
#define  STEP_OFF()         GpioSetBits(GPIOA, GPIO_Pin_1)


void pin_init ()
{
  GPIO_InitTypeDef         GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  gpioinit (GPIOB, &GPIO_InitStructure);
  GpioClrBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);

  // PA1 as MicrostepDriver enable (1:Disable  0:Enable)
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  gpioinit (GPIOA, &GPIO_InitStructure);
  STEP_ON ();
}

#define  STEP_DEGREE_90    400
#define  STEP_DEGREE_GAP   150
#define  STEP_CENTER_ADJ   (12)
#define  STEP_DEGREE_GAP_F (STEP_DEGREE_GAP + 4)
#define  STEP_DEGREE_GAP_B (STEP_DEGREE_GAP + 2)
#define  STEP_DELAY_US     120

#define  SERVO_OPEN         TIM2->CCR1 = SERVO_OPEN_VAL
#define  SERVO_CLOSE        TIM2->CCR1 = SERVO_CLOSE_VAL

short  step_count;
char   motor_dir;
char   servo_state;

//                                 Front        Right        Left
const uint16_t motor_dir_pin[3] = {GPIO_Pin_12, GPIO_Pin_15, GPIO_Pin_0};
const uint16_t motor_clk_pin[3] = {GPIO_Pin_13, GPIO_Pin_14, GPIO_Pin_1};

void move_step_f (short step)
{
  step_count += step;

  if (step >= 0) {
    if (motor_dir == 1) {
      GpioClrBits (GPIOB, motor_dir_pin[0]);
      motor_dir = 0;
    }
  } else {
    if (motor_dir == 0) {
      GpioSetBits (GPIOB, motor_dir_pin[0]);
      motor_dir = 1;
    }
    step = -step;
  }
  while (step > 0) {
    GpioSetBits(GPIOB, motor_clk_pin[0]);
    DelayUs (STEP_DELAY_US);
    GpioClrBits(GPIOB, motor_clk_pin[0]);
    DelayUs (STEP_DELAY_US);
    step--;
  }
}

void rotate_f (int8_t repeat)
{
  if (repeat > 0) {
    move_step_f (STEP_DEGREE_GAP_F);
  } else {
    move_step_f (-STEP_DEGREE_GAP_B);
  }
  move_step_f (STEP_DEGREE_90 * repeat);
  if (repeat > 0) {
    move_step_f (-STEP_DEGREE_GAP_F);
  } else {
    move_step_f (STEP_DEGREE_GAP_B);
  }
}

void servo_close ()
{
  SERVO_CLOSE;
  servo_state = 1;
}

void servo_open ()
{
  SERVO_OPEN;
  servo_state = 0;
}


void rotate_s (uint8_t mask, int8_t repeat)
{
  short  step;

  if (mask != 3) {
    // single side move, block middle row
    if (servo_state == 0) {
      servo_close ();
      DelayMs(300);
    }
  } else {
    if (servo_state == 1) {
      servo_open ();
      DelayMs(300);
    }
  }

  if (repeat >= 0) {
    if (mask & 1)  GpioSetBits (GPIOB, motor_dir_pin[1]);
    if (mask & 2)  GpioClrBits (GPIOB, motor_dir_pin[2]);
  } else {
    if (mask & 1)  GpioClrBits (GPIOB, motor_dir_pin[1]);
    if (mask & 2)  GpioSetBits (GPIOB, motor_dir_pin[2]);
    repeat = -repeat;
  }

  step = repeat * STEP_DEGREE_90;
  while (step > 0) {
    if (mask & 1)   GpioSetBits(GPIOB, motor_clk_pin[1]);
    if (mask & 2)   GpioSetBits(GPIOB, motor_clk_pin[2]);
    DelayUs (STEP_DELAY_US);
    if (mask & 1)   GpioClrBits(GPIOB, motor_clk_pin[1]);
    if (mask & 2)   GpioClrBits(GPIOB, motor_clk_pin[2]);
    DelayUs (STEP_DELAY_US);
    step--;
  }
}

void run (char *step)
{
  int8_t   rt;
  int8_t   mv;
  uint8_t  mask;

  rt = 0;
  mv = (step[1] - '0') & 3;

  if (step[0] == 'E') {
    if (mv & 1) {
      STEP_ON ();
    } else {
      STEP_OFF ();
    }
    return;
  } else if (step[0] == 'X') {
    return;
  } else if (step[0] == 'M') {
    rotate_f (mv);
    DelayMs  (200);
    return;
  } else if (step[0] == 'N') {
    rotate_f (-mv);
    DelayMs  (200);
    return;
  } else if (step[0] == 'T') {
    rotate_s (3, mv);
    DelayMs  (200);
    return;
  } else if (step[0] == 'B') {
    rt =  2;
    //mv = -mv;
  } else if (step[0] == 'U') {
    rt =  1;
  } else if (step[0] == 'D') {
    rt = -1;
    //mv = -mv;
  } else if ((step[0] == 'R') || (step[0] == 'L')) {
    if (step[0] == 'R') {
      mask = 1;
      mv   = -mv;
    } else {
      mask = 2;
      mv   = -(4 - mv);
    }
    rotate_s (mask, mv);
    DelayMs (200);
    return;
  }

  if (rt != 0)  {
    rotate_s (3, rt);
    DelayMs (200);
  }

  rotate_f (mv);
  DelayMs (200);

  if (rt != 0)  {
    rotate_s (3, -rt);
    DelayMs (200);
  }
}

bool run_steps (char *steps, bool space)
{
  bool    loop;
  uint8_t idx;

  idx  = 0;
  loop = true;
  while (loop) {
    if (!((steps[idx] == 'U') || (steps[idx] == 'D') || (steps[idx] == 'F') || \
          (steps[idx] == 'B') || (steps[idx] == 'R') || (steps[idx] == 'L') || \
          (steps[idx] == 'T'))) {
      break;
    }

    if (!((steps[idx+1] >= '1') && (steps[idx+1] <= '3'))) {
      break;
    }

    run (steps + idx);

    if (space) {
      if (steps[idx+2] != ' ') {
        loop = false;
        break;
      }
      idx += 3;
    } else {
      idx += 2;
    }
  }

  servo_open ();
  DelayMs(500);

  return loop;
}

#define  EP_MAX_SIZE    64

void reset ()
{
  run_steps ("", 0);
}

void Main ()
{
  char      ch;
  BYTE     *buf;
  BYTE      loop;
  BYTE      plen;
  WORD      rxlen;

  TICK_MARKER  Marker = {0};

  SysTickInit ();

  pin_init ();

  pwm_init ();

  printf ("Rubik's Cube\n");
  printf ("Waiting for USB command\n");

  step_count  = 0;
  motor_dir   = 1;
  servo_state = 0;

  // Drain all packets
  while (UsbGetRxBuf(&rxlen))
  UsbFreeRxBuf();

  loop = 1;
  while (loop) {

    if (CHECK_MARK(1, 1)) {
      // 10 * (1<<1) = 20ms
      TOGGLE_MARK(1);
    }

    plen = 0xFF;
    buf = UsbGetRxBuf(&rxlen);
    if (buf) {

      plen = (BYTE)rxlen;
      if (plen == EP_MAX_SIZE) { // Data packet
        printf ("RX SEQ: %s\n", buf);
        run_steps ((char *)buf, 0);
      } else if (plen == CMD_PACKET_LEN) { // Command packet
        printf ("RX CMD: %s\n", buf);
        if (buf[0] == '@') {
          switch (buf[1]) {
          case 'Q':
            plen = 0xF0;
            break;
          case 'R':
            reset ();
            break;
          case 'M':
            run ((char *)buf+2);
            break;
          default:
            break;
          }
        }
      }
    }

    if ((plen == 0xF0) || haschar()) {
      if (haschar()) {
        ch = getchar();
        if (ch == 27) {
          loop = 0;
        } else {
          if (ch == 'q')  {
            servo_open ();
          }
          if (ch == 'w')  {
            servo_close ();
          }

          if (ch == 'r') {
            rotate_f (1);
          }
          if (ch == 'l') {
            rotate_f (-1);
          }

          if (ch == 's')  {
            rotate_s (1, 1);
          }
          if (ch == 't')  {
            rotate_s (1, -1);
          }

          if (ch == 'u')  {
            rotate_s (2, 1);
          }
          if (ch == 'v')  {
            rotate_s (2, -1);
          }

          if (ch == 'x')  {
            rotate_s (3, 1);
          }
          if (ch == 'y')  {
            rotate_s (3, -1);
          }
        }
      }
    }

    if (plen != 0xFF) {
      UsbFreeRxBuf();
    }
  }

  servo_open ();
  STEP_OFF();

  printf ("Quit\n");

  return;
}

