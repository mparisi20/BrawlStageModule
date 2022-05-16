_ctors = ADDR(.ctors);
_dtors = ADDR(.dtors);
SECTIONS
{
    .init : {}
    .text : { *(.text*)}
    .ctors : {*(.ctors*)}
    .dtors : {*(.dtors*)}
    .rodata : { *(.rodata*)}
    .data : {*(.data*)}
    .bss : {*(.bss*)}
}