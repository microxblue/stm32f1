/* File: startup_ARMCM3.S
 * Purpose: startup file for Cortex-M3 devices. Should use with
 *   GCC for ARM Embedded Processors
 * Version: V1.2
 * Date: 15 Nov 2011
 *
 * Copyright (c) 2011, ARM Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the ARM Limited nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ARM LIMITED BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 * History:
 *
 * 12.06.2012  mifi  Expand for stm32f103cb.
 *                   Added external interrupts.
 *                   Added clearing of BSS segment.
 ******************************************************************************/
    .syntax unified
    .arch armv7-m

    .section .stack
    .align 3
    .equ      Stack_Size, 0x400
    .globl    __StackTop
    .globl    __StackLimit
__StackLimit:
    .space    Stack_Size
    .size __StackLimit, . - __StackLimit
__StackTop:
    .size __StackTop, . - __StackTop


    .section .isr_vector
    .align 2
    .globl __isr_vector
__isr_vector:
    .long    __StackTop            /* 00 Top of Stack */
    .long    Reset_Handler         /* 01 Reset Handler */
    .long    NMI_Handler           /* 02 NMI Handler */
    .long    HardFault_Handler     /* 03 Hard Fault Handler */
    .long    MemManage_Handler     /* 04 MPU Fault Handler */
    .long    BusFault_Handler      /* 05 Bus Fault Handler */
    .long    UsageFault_Handler    /* 06 Usage Fault Handler */
    .long    0                     /* 07 Reserved */
    .long    0                     /* 08 Reserved */
    .long    0                     /* 09 Reserved */
    .long    0                     /* 10 Reserved */
    .long    SVC_Handler           /* 11 SVCall Handler */
    .long    DebugMon_Handler      /* 12 Debug Monitor Handler */
    .long    0                     /* 13 Reserved */
    .long    PendSV_Handler        /* 14 PendSV Handler */
    .long    SysTick_Handler       /* 15 SysTick Handler */

    /* External interrupts */
    .word     WWDG_IRQHandler                   /* 16 Window WatchDog              */
    .word     PVD_IRQHandler                    /* 17 PVD through EXTI Line detection */
    .word     TAMP_STAMP_IRQHandler             /* 18 Tamper and TimeStamps through the EXTI line */
    .word     RTC_WKUP_IRQHandler               /* 19 RTC Wakeup through the EXTI line */
    .word     FLASH_IRQHandler                  /* 20 FLASH                        */
    .word     RCC_IRQHandler                    /* 21 RCC                          */
    .word     EXTI0_IRQHandler                  /* 22 EXTI Line0                   */
    .word     EXTI1_IRQHandler                  /* 23 EXTI Line1                   */
    .word     EXTI2_IRQHandler                  /* 24 EXTI Line2                   */
    .word     EXTI3_IRQHandler                  /* 25 EXTI Line3                   */
    .word     EXTI4_IRQHandler                  /* 26 EXTI Line4                   */
    .word     DMA1_Stream0_IRQHandler           /* 27 DMA1 Stream 0                */
    .word     DMA1_Stream1_IRQHandler           /* 28 DMA1 Stream 1                */
    .word     DMA1_Stream2_IRQHandler           /* 29 DMA1 Stream 2                */
    .word     DMA1_Stream3_IRQHandler           /* 30 DMA1 Stream 3                */
    .word     DMA1_Stream4_IRQHandler           /* 31 DMA1 Stream 4                */
    .word     DMA1_Stream5_IRQHandler           /* 32 DMA1 Stream 5                */
    .word     DMA1_Stream6_IRQHandler           /* 33 DMA1 Stream 6                */
    .word     ADC_IRQHandler                    /* 34 ADC1, ADC2 and ADC3s         */
    .word     USB_HP_CAN1_TX_IRQHandler         /* 35 CAN1 TX                      */
    .word     USB_LP_CAN1_RX0_IRQHandler        /* 36 CAN1 RX0                     */
    .word     CAN1_RX1_IRQHandler               /* 37 CAN1 RX1                     */
    .word     CAN1_SCE_IRQHandler               /* 38 CAN1 SCE                     */
    .word     EXTI9_5_IRQHandler                /* 39 External Line[9:5]s          */
    .word     TIM1_BRK_TIM9_IRQHandler          /* 40 TIM1 Break and TIM9          */
    .word     TIM1_UP_TIM10_IRQHandler          /* 41 TIM1 Update and TIM10        */
    .word     TIM1_TRG_COM_TIM11_IRQHandler     /* 42 TIM1 Trigger and Commutation and TIM11 */
    .word     TIM1_CC_IRQHandler                /* 43 TIM1 Capture Compare         */
    .word     TIM2_IRQHandler                   /* 44 TIM2                         */
    .word     TIM3_IRQHandler                   /* 45 TIM3                         */
    .word     TIM4_IRQHandler                   /* 46 TIM4                         */
    .word     I2C1_EV_IRQHandler                /* 47 I2C1 Event                   */
    .word     I2C1_ER_IRQHandler                /* 48 I2C1 Error                   */
    .word     I2C2_EV_IRQHandler                /* 49 I2C2 Event                   */
    .word     I2C2_ER_IRQHandler                /* 50 I2C2 Error                   */
    .word     SPI1_IRQHandler                   /* 51 SPI1                         */
    .word     SPI2_IRQHandler                   /* 52 SPI2                         */
    .word     USART1_IRQHandler                 /* 53 USART1                       */
    .word     USART2_IRQHandler                 /* 54 USART2                       */
    .word     USART3_IRQHandler                 /* 55 USART3                       */
    .word     EXTI15_10_IRQHandler              /* 56 External Line[15:10]s        */
    .word     RTC_Alarm_IRQHandler              /* 57 RTC Alarm (A and B) through EXTI Line */

    .size    __isr_vector, . - __isr_vector

    .text
    .thumb
    .thumb_func
    .align 2
    .globl    Reset_Handler
    .type     Reset_Handler, %function
Reset_Handler:
/*
 * Loop to copy data from read only memory to RAM. The ranges
 * of copy from/to are specified by following symbols evaluated in
 * linker script.
 * __etext: End of code section, i.e., begin of data sections to copy from.
 * __data_start__/__data_end__: RAM address range that data should be
 * copied to. Both must be aligned to 4 bytes boundary.
 */
#if 1
    ldr    r1, =__StackTop
    msr    msp, r1
#endif

    ldr    r1, =__etext
    ldr    r2, =__data_start__
    ldr    r3, =__data_end__

#if 0
/*
 * Here are two copies of loop implemenations. First one favors code size
 * and the second one favors performance. Default uses the first one.
 * Change to "#if 0" to use the second one
 */
.flash_to_ram_loop1:
    cmp    r2, r3
    ittt   lt
    ldrlt  r0, [r1], #4
    strlt  r0, [r2], #4
    blt    .flash_to_ram_loop1
#else
    subs   r3, r2
    ble    .flash_to_ram_loop_end
.flash_to_ram_loop2:
    subs   r3, #4
    ldr    r0, [r1, r3]
    str    r0, [r2, r3]
    bgt    .flash_to_ram_loop2
.flash_to_ram_loop_end:
#endif

#ifndef __NO_SYSTEM_INIT
    ldr    r0, =SystemInit
    blx    r0
#endif

/*
 * Clear the BSS segment
 */
    ldr    r0, =0
    ldr    r1, =__bss_start__
    ldr    r2, =__bss_end__

clear_bss_loop:
    cmp    r1, r2
    beq    clear_bss_loop_end
    str    r0, [r1], #4
    b      clear_bss_loop
clear_bss_loop_end:

/*
 * Jump to main
 */
    ldr    r0, =main
    bx     r0

/*
 * Exit loop
 */
exit_loop:
    nop
    b      exit_loop

    .pool
    .size Reset_Handler, . - Reset_Handler

/*    Macro to define default handlers. Default handler
 *    will be weak symbol and just dead loops. They can be
 *    overwritten by other handlers */
    .macro    def_default_handler    handler_name
    .align 1
    .thumb_func
    .weak    \handler_name
    .type    \handler_name, %function
\handler_name :
    b    .
    .size    \handler_name, . - \handler_name
    .endm

    def_default_handler    NMI_Handler
    def_default_handler    HardFault_Handler
    def_default_handler    MemManage_Handler
    def_default_handler    BusFault_Handler
    def_default_handler    UsageFault_Handler
    def_default_handler    SVC_Handler
    def_default_handler    DebugMon_Handler
    def_default_handler    PendSV_Handler
    def_default_handler    SysTick_Handler

    /* External interrupts */
    def_default_handler    WWDG_IRQHandler
    def_default_handler    PVD_IRQHandler
    def_default_handler    TAMP_STAMP_IRQHandler
    def_default_handler    RTC_WKUP_IRQHandler
    def_default_handler    FLASH_IRQHandler
    def_default_handler    RCC_IRQHandler
    def_default_handler    EXTI0_IRQHandler
    def_default_handler    EXTI1_IRQHandler
    def_default_handler    EXTI2_IRQHandler
    def_default_handler    EXTI3_IRQHandler
    def_default_handler    EXTI4_IRQHandler
    def_default_handler    DMA1_Stream0_IRQHandler
    def_default_handler    DMA1_Stream1_IRQHandler
    def_default_handler    DMA1_Stream2_IRQHandler
    def_default_handler    DMA1_Stream3_IRQHandler
    def_default_handler    DMA1_Stream4_IRQHandler
    def_default_handler    DMA1_Stream5_IRQHandler
    def_default_handler    DMA1_Stream6_IRQHandler
    def_default_handler    ADC_IRQHandler
    def_default_handler    USB_HP_CAN1_TX_IRQHandler
    def_default_handler    USB_LP_CAN1_RX0_IRQHandler
    def_default_handler    CAN1_RX1_IRQHandler
    def_default_handler    CAN1_SCE_IRQHandler
    def_default_handler    EXTI9_5_IRQHandler
    def_default_handler    TIM1_BRK_TIM9_IRQHandler
    def_default_handler    TIM1_UP_TIM10_IRQHandler
    def_default_handler    TIM1_TRG_COM_TIM11_IRQHandler
    def_default_handler    TIM1_CC_IRQHandler
    def_default_handler    TIM2_IRQHandler
    def_default_handler    TIM3_IRQHandler
    def_default_handler    TIM4_IRQHandler
    def_default_handler    I2C1_EV_IRQHandler
    def_default_handler    I2C1_ER_IRQHandler
    def_default_handler    I2C2_EV_IRQHandler
    def_default_handler    I2C2_ER_IRQHandler
    def_default_handler    SPI1_IRQHandler
    def_default_handler    SPI2_IRQHandler
    def_default_handler    USART1_IRQHandler
    def_default_handler    USART2_IRQHandler
    def_default_handler    USART3_IRQHandler
    def_default_handler    EXTI15_10_IRQHandler
    def_default_handler    RTC_Alarm_IRQHandler
    def_default_handler    OTG_FS_WKUP_IRQHandler

   .end
   /*** EOF ***/