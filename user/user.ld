SECTIONS
{
    ENTRY(main)

    . = 0x8000000000;
    .text : { *(.entry) *(.text*) }
    .rodata : { *(.rodata*) }
    .bss : { *(.bss) }
    /DISCARD/ : { *(.eh_frame) }
    _user_end = .;
}
