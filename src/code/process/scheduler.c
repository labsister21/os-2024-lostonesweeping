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

// void scheduler_switch_to_next_process(void){
//     struct ProcessControlBlock* current_process = process_get_current_running_pcb_pointer();

//     if(current_process != NULL){
//         current_process->metadata.state = Waiting; 
//     }
    
//     int next_process_index = -1; 
//     for(int i = 0; i < PROCESS_COUNT_MAX; i++){
//         if(_process_list[i].metadata.state == Waiting && &_process_list[i] != current_process){
//             next_process_index = i;
//             break;
//         }
//     }

//     if(next_process_index == -1){
//         if (current_process != NULL) {
//             current_process->metadata.state = Running;
//             paging_use_page_directory(current_process->context.page_directory_virtual_addr);
//             process_context_switch(current_process->context);
//         } else {
//             while (1) __asm__("hlt");
//         }
//     }

//     _process_list[next_process_index].metadata.state = Running; 
//     paging_use_page_directory(_process_list[next_process_index].context.page_directory_virtual_addr);
//     process_context_switch(_process_list[next_process_index].context);
// }

void scheduler_switch_to_next_process(void){
    struct ProcessControlBlock* current_process = process_get_current_running_pcb_pointer();

    if(current_process != NULL){
        current_process->metadata.state = Waiting; 
    }
    
    int current_index = 0; 
    while(current_index < PROCESS_COUNT_MAX){
        if(_process_list[current_index++].metadata.state == Running)  break;
    }

    int next_index = current_index + 1;
    while(true) {
        if(next_index == current_index) break; 
        if(_process_list[next_index].metadata.state == Waiting) {
            _process_list[current_index].metadata.state = Waiting;
            break; 
        }
        next_index = (next_index + 1) % PROCESS_COUNT_MAX;
    } 


    if(next_index == current_index){
        paging_use_page_directory(current_process->context.page_directory_virtual_addr); 
        process_context_switch(current_process->context);
    }
    else{
        _process_list[next_index].metadata.state = Running; 
        paging_use_page_directory(_process_list[next_index].context.page_directory_virtual_addr);
        process_context_switch(_process_list[next_index].context);
    }
}