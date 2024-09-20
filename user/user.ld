SECTIONS
{
    ENTRY(_start)

    . = 0x8000000000;
    .entry : { *(.entry) }
    .text : { *(.text*) }

    /*
    Must be aligned to page size, or ld will put it in the same segment as .text, and
    we'll get a warning about a LOAD segment with RWX permissions
    */
    .data : ALIGN(4096) { *(.rodata*) }
    .rodata : { *(.rodata*) }
    .bss : { *(.bss) }

    /DISCARD/ : { *(.eh_frame) }
    _user_end = .;
}
