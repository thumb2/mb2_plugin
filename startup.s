    .syntax unified
    .arch armv6-m
    
    .text
    .thumb
    .thumb_func
    .align 1
    .globl _start
_start:
    push {r0, r1}
    
    ldr r1, =__etext
    ldr r2, =__data_start__
    ldr r3, =__bss_start__

    subs r3, r2
    ble copy_done

copy_loop:
    
    subs r3, #4
    ldr r0, [r1,r3]
    str r0, [r2,r3]
    bgt copy_loop

copy_done:
    pop {r0, r1}    
    push {lr}
    bl plugin_init
    pop {pc}

    
