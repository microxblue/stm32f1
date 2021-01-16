MEMORY
{
  FLASH (rx) : ORIGIN = @FLASHBASE, LENGTH = @FLASHSIZE
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 4K - 0x400
}

/* Section Definitions */
SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector .isr_vector.*))
        *(.text .text.* .gnu.linkonce.t.*)
        *(.glue_7t) *(.glue_7)
        *(.rodata .rodata* .gnu.linkonce.r.*)
    } >FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } >FLASH

    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } >FLASH

    . = ALIGN(4);
    _etext = .;
    __etext = .;
    _sidata = .;

    .data : AT (_etext)
    {
        __data_start__ = .;
        *(.data .data.*)
        . = ALIGN(4);
        __data_end__ = .;
    } >RAM

    /* .bss section which is used for uninitialized data */
    .bss (NOLOAD) :
    {
        __bss_start__ = . ;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
         __bss_end__ = . ;
    } >RAM

    /* stack section */
    .co_stack (NOLOAD):
    {
        . = ALIGN(8);
		    *(.stack)
    } >RAM

  	__StackTop   = ORIGIN(RAM) + LENGTH(RAM);
    __StackLimit = __StackTop - SIZEOF(.co_stack);
	  PROVIDE(__stack = __StackTop);

    . = ALIGN(4);
    _end = . ;

    /* Check if data + stack exceeds RAM limit */
	  ASSERT(__StackLimit >= __bss_end__, "region RAM overflowed with stack")
}