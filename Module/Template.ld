MEMORY
{
  FLASH (rx) : ORIGIN = @FLASHBASE, LENGTH = @FLASHSIZE
  RAM (rwx)  : ORIGIN = @SRAMBASE, LENGTH = @SRAMSIZE
}

/* Section Definitions */
SECTIONS
{
    .text :
    {
        *(.header)
        *(.entry)
        *(.text .text.* .gnu.linkonce.t.*)
        *(.glue_7t) *(.glue_7)
        *(.rodata .rodata* .gnu.linkonce.r.*)
    } >@CODEREGION

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } >@CODEREGION

    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } >@CODEREGION

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

    _end = . ;
}