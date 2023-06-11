SECTIONS
{
    ENTRY(kmain)

    . = 0x7E00;
    .text : { *(.entry) *(.text*) }
    .rodata : { *(.rodata*) }
    .bss : { *(.bss) }
    /DISCARD/ : { *(.eh_frame) }
    _kernelEnd = .;
}
