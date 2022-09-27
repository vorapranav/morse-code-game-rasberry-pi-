#include "hardware/regs/addressmap.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/regs/m0plus.h"
#include "hardware/regs/timer.h"

.syntax unified
.cpu    cortex-m0plus
.thumb
.global main_asm
.align  4


.equ    DFLT_STATE_STRT, 1            @ Specify the value to start flashing
.equ    DFLT_STATE_STOP, 0            @ Specify the value to stop flashing
.equ    DFLT_ALARM_TIME, 1000000      @ Specify the default alarm timeout
.equ    ALRM_TIME,       2500000
.equ    GPIO_BTN_EN_MSK, 0x00400000   @ Bit-22 for falling-edge event on GP21

.equ    GPIO_BTN_EN,     21           @ Specify pin for the "enter" button
.equ    GPIO_BTN_EN_MSK, 0x00400000   @ Bit-22 for falling-edge event on GP21
.equ    GPIO_DIR_IN,     0            @ Specify input direction for a GPIO pin
.equ    GPIO_ISR_OFFSET, 0x74         @ GPIO is int #13 (vector table entry 29)

.equ    ALRM_ISR_OFFSET, 0x40         @ ALARM0 is int #0 (vector table entry 16)


@ Entry point to the ASM portion of the program
main_asm:
   bl   init_btns
   bl   install_alrm_isr
   bl   install_gpio_isr

loop:
    bl  set_alarm
    wfi                     @ Wait for interrupt
    b   loop                @ Infinite loop

@ Sets up a pin with the provided pin number in r0 and the pin direction in r1
init_btns:
    push    {lr}
    movs    r0, #GPIO_BTN_EN            
    bl      asm_gpio_init              
    movs    r0, #GPIO_BTN_EN            
    movs    r1, #GPIO_DIR_IN            
    bl      asm_gpio_set_dir
    movs    r0, #GPIO_BTN_EN
    bl      asm_gpio_set_irq
    pop     {pc}

install_alrm_isr:
    @ Get the address of the RAM vector table using the (PPB_BASE + M0PLUS_VTOR_OFFSET) register
    ldr     r2, =(PPB_BASE + M0PLUS_VTOR_OFFSET)        
    ldr     r1, [r2]                                    
    movs    r2, #ALRM_ISR_OFFSET                        
    add     r2, r1 
    @ Store the address of the alrm_isr handler to the correct offset for ALARM0 in the vector table                                     
    ldr     r0, =alarm_isr                               
    str     r0, [r2] 
    movs    r0, #1  
    @ Disable the ALARM0 IRQ by writing the correct value to (PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET)                   
    ldr     r1, =(PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET)
    str     r0, [r1]
    @ Enable the ALARM0 IRQ by writing the correct value to (PPB_BASE + M0PLUS_NVIC_ISER_OFFSET)
    ldr     r1, =(PPB_BASE + M0PLUS_NVIC_ISER_OFFSET)
    str     r0, [r1] 
    @ Exit subroutine   
    bx      lr 

@ Sets up the gpio_isr in the RAM vector table and enables the correct interrupt
install_gpio_isr:
    @ Get the address of the RAM vector table using the (PPB_BASE + M0PLUS_VTOR_OFFSET) register
    ldr     r2, =(PPB_BASE + M0PLUS_VTOR_OFFSET)        
    ldr     r1, [r2]                                    
    movs    r2, #GPIO_ISR_OFFSET                        
    add     r2, r1                                      
    ldr     r0, =gpio_isr
    @ Store the address of the gpio_isr handler to the correct offset for GPIO in the vector table
    str     r0, [r2] 

    ldr     r0, =8192                                     
    @ Disable the GPIO IRQ by writing the correct value to (PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET)
    ldr     r1, =(PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET)
    str     r0, [r1]
    @ Enable the GPIO IRQ by writing the correct value to (PPB_BASE + M0PLUS_NVIC_ISER_OFFSET)
    ldr     r1, =(PPB_BASE + M0PLUS_NVIC_ISER_OFFSET)
    str     r0, [r1]
    @ Exit subroutine    
    bx      lr 

@ Arm/reset Alarm
set_alarm:
    push    {lr}
    ldr     r2, =ALRM_TIME
    ldr     r1, =(TIMER_BASE + TIMER_TIMELR_OFFSET)
    ldr     r0, [r1]
    add     r0, r2
    ldr     r1, =(TIMER_BASE + TIMER_ALARM0_OFFSET)
    str     r0, [r1]
    movs    r0, #1
    ldr     r1, =(TIMER_BASE + TIMER_INTE_OFFSET)
    str     r0, [r1]
    pop     {pc}

@ Alarm ISR - Handles input timeout
.thumb_func
alarm_isr:
    push    {lr}
    movs    r0, #1
    ldr     r1, =(TIMER_BASE + TIMER_INTR_OFFSET)
    str     r0, [r1]
    ldr     r0, =char
    movs    r1, #0
    str     r1, [r0]
    ldr     r0, =2
    bl      Dot_or_Dash
    pop     {pc}

@ GPIO Button ISR - Handles user input
.thumb_func
gpio_isr:
    push    {lr}
    ldr     r1, =(IO_BANK0_BASE + IO_BANK0_INTR2_OFFSET)
    ldr     r2, [r1]
    str     r2, [r1]
    ldr     r1, =GPIO_BTN_EN_MSK
    ands    r2, r1
    cmp     r1, r2
    ldr     r1, =0
    bne     falling_edge

rising_edge:
    ldr     r1, =(TIMER_BASE + TIMER_TIMELR_OFFSET)
    ldr     r0, [r1]
    ldr     r1, =time                                   @ Get the time store it in r0
    str     r0, [r1]
    b       gpio_isr_done

falling_edge:
    ldr     r1, =(TIMER_BASE + TIMER_TIMELR_OFFSET)
    ldr     r0, [r1]
    ldr     r1, =time                                   @ Get the time store it in r1
    ldr     r1, [r1]
    subs    r0, r1                                      @ Get the difference 
    ldr     r1, =500000 
    cmp     r0, r1                                      @ If the difference is less than 500000 microseconds branch to dot else to dash 
    blt     dot

dash:
    ldr     r0, =1 
    bl      Dot_or_Dash                                 @ Branch to C function for a dash
    bl      gpio_isr_done

dot:
    ldr     r0, =0
    bl      Dot_or_Dash                                 @ Branch to C function for dot
    bl      gpio_isr_done
    
gpio_isr_done:
    pop     {pc}                                        @ Pop value off pc register

.data
time: .word     0
char: .word     0
morse: .asciz   "     "
button_addr: .word 1
  
