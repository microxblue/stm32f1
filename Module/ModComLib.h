#ifndef MOD_COM_LIB_H
#define MOD_COM_LIB_H

typedef struct {
    UINT32  Event0 :  1;
    UINT32  Event1 :  1;
    UINT32  Event2 :  1;
    UINT32  Event3 :  1;
    UINT32  Unused : 28;
} TICK_MARKER;

#define  CHECK_MARK(e, t)  ((SysTickCnt ^ (Marker.Event##e<<(t))) & (1<<(t)))
#define  TOGGLE_MARK(e)    (Marker.Event##e ^= 1)

#define  GpioGetBits(GPIOx, GPIO_Pin)   (GPIOx->IDR & GPIO_Pin)
#define  GpioSetBits(GPIOx, GPIO_Pin)   (GPIOx->BSRR = GPIO_Pin)
#define  GpioClrBits(GPIOx, GPIO_Pin)   (GPIOx->BRR  = GPIO_Pin)

#endif






