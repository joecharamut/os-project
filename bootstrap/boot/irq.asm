%macro irq 1
    global irq%1
    extern irq%1_handler
    irq%1:
        pusha
        call irq%1_handler
        popa
        iret
%endmacro

%assign i 0
%rep 16
    irq i
    %assign i i+1
%endrep

global load_idt
load_idt:
    cli
    mov edx, [esp + 4]
    lidt [edx]
    sti
    ret
