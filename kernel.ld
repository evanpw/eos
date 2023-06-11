SECTIONS
{
    ENTRY(kmain)

    . = 0x7E00;
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .bss : { *(.bss) }
    /DISCARD/ : { *(.eh_frame) }
    _kernelEnd = .;
}
