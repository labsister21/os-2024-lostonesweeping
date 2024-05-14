#include "header/process/scheduler.h"
#include "header/cpu/portio.h"
#include "stdint.h"
#include "header/stdlib/string.h"


void activate_timer_interrupt(void) {
    __asm__ volatile("cli");
    // Setup how often PIT fire
    uint32_t pit_timer_counter_to_fire = PIT_TIMER_COUNTER;
    out(PIT_COMMAND_REGISTER_PIO, PIT_COMMAND_VALUE);
    out(PIT_CHANNEL_0_DATA_PIO, (uint8_t) (pit_timer_counter_to_fire & 0xFF));
    out(PIT_CHANNEL_0_DATA_PIO, (uint8_t) ((pit_timer_counter_to_fire >> 8) & 0xFF));

    // Activate the interrupt
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_TIMER));
}

void scheduler_init(void) {
    activate_timer_interrupt(); 

    int idx = 0;
    for(int i = 0; i < PROCESS_COUNT_MAX; i++){
        if(_process_list[i].metadata.state == Waiting) {
            idx = i;
            break;
        }
    } 

    if(idx == PROCESS_COUNT_MAX){
        return;
    }else{
        _process_list[idx].metadata.state = Running;
    }
};

void scheduler_save_context_to_current_running_pcb(struct Context ctx){
    struct ProcessControlBlock* current_process = process_get_current_running_pcb_pointer();
    if (current_process != NULL) {
        current_process->context = ctx;
    }
}

void scheduler_switch_to_next_process(void){
    struct ProcessControlBlock* current_process = process_get_current_running_pcb_pointer();

    if(current_process != NULL){
        struct Context saved_ctx; 
        scheduler_save_context_to_current_running_pcb(saved_ctx); 
        current_process->metadata.state = Waiting; 
    }
    int next_process_index = -1; 
    for(int i = 0; i < PROCESS_COUNT_MAX; i++){
        if(_process_list[i].metadata.state == Waiting){
            next_process_index = i;
            break;
        }
    }

    if(next_process_index == -1){
        if (current_process != NULL) {
            current_process->metadata.state = Running;
            process_context_switch(current_process->context);
        } else {
            while (1) __asm__("hlt");
        }
    }

    _process_list[next_process_index].metadata.state = Running; 
    process_context_switch(_process_list[next_process_index].context);
}
