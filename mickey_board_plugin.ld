/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x2d500, LENGTH = 0x12500
  RAM (rwx) :  ORIGIN = 0x20006000, LENGTH = 0x2000
}

ENTRY(_start)

SECTIONS
{
    .text :
    {
        *(.text*)
    } > FLASH

    . = ALIGN(4);
    __etext = .;

    .data : AT (__etext)
    {
        PROVIDE(__data_start__ = .);
        *(.data*)

        . = ALIGN(4);
        /* All data end */
        __data_end__ = .;

    } > RAM

    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
        end = .;
    } > RAM
    
}


