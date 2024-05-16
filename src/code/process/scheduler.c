#include "header/process/scheduler.h"
#include "header/cpu/portio.h"
#include "stdint.h"
#include "header/stdlib/string.h"


/**
 * gatau ini ngapain
*/
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

/**
 * DOC: 
 * scheduler init ini buat inisialisasi scheduler yah untuk sekarang atau sampe seterusnya 
 * dia langsung milih si shell 
*/
void scheduler_init(void) {
    activate_timer_interrupt(); 

    /**
     * ini cari yang waiting 
    */
    int idx = 0;
    for(int i = 0; i < PROCESS_COUNT_MAX; i++){
        if(_process_list[i].metadata.state == Waiting) {
            idx = i;
            break;
        }
    } 

    /**
     * kalo ga ketemu yang waiting yaudah out
    */
    if(idx == PROCESS_COUNT_MAX){
        return;
    }else{
        /**
         * kalao misalkan ketemu atur state jadi running desu
         * sisanya? urusan context switch assembly i hate assembly 
        */
        _process_list[idx].metadata.state = Running;
    }
};

void scheduler_save_context_to_current_running_pcb(struct Context ctx){
    /**
     * ini buat ganti process jadi nyimpen context yang sekarang lalu ganti ke process yang lain 
    */
    struct ProcessControlBlock* current_process = process_get_current_running_pcb_pointer();
    if (current_process != NULL) {
        current_process->context = ctx;
    }
}


/**
 * ganti proses
 * you know? fck algorithm just puterin aja semua process, biar semua proses ke-"proses" dan 
 * tidak ada yang tidak ke-"proses" 
 * 
 * waktunya mepet desu, jadi yaudah gitu oke
*/
void scheduler_switch_to_next_process(void){
    /**
     * ini ambil current_process  yang berjalan 
    */
    struct ProcessControlBlock* current_process = process_get_current_running_pcb_pointer();

    /**
     * atur processnya jadi waiting
    */
    int current_index = 0; 
    if(current_process != NULL){
        while(current_index < PROCESS_COUNT_MAX){
            if(_process_list[current_index].metadata.state == Running){
                break;
            }
            current_index++;
        }
        current_process->metadata.state = Waiting; 
    }
    
    /**
     * cari proses selanjutnya yang statusnya Waiting
    */
    int next_index = current_index + 1;
    while(true) {
        if(next_index == current_index) break; 
        if(_process_list[next_index].metadata.state == Waiting) {
            _process_list[current_index].metadata.state = Waiting;
            break; 
        }
        next_index = (next_index + 1) % PROCESS_COUNT_MAX;
    } 

    /**
     * kalo misalkan next_index == current_index atau balik ke awal yaudah gada proses lain selain shell 
     * jadi pake shell lagi lmao 
    */
    if(next_index == current_index){
        current_process->metadata.state = Running;
        paging_use_page_directory(current_process->context.page_directory_virtual_addr); 
        process_context_switch(current_process->context);
    }
    /**
     * kalo ada proses lain masuk kesini 
    */
    else{
        _process_list[next_index].metadata.state = Running; 
        paging_use_page_directory(_process_list[next_index].context.page_directory_virtual_addr);
        process_context_switch(_process_list[next_index].context);
    }
}