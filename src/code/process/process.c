#include "header/process/process.h"
#include "header/memory/paging.h"
#include "header/stdlib/string.h"
#include "header/cpu/gdt.h"


/**
 * ini pindah harusnya disini atau di process_manager_state? 
 * tau ah aku taruh di process_magnager_state dulubut wait... 
*/
// typedef struct {
//     uint32_t pid; 
// } Process_Control_Block;

// i dont know what is this, saya assume nulis begini karena di program dibawah
/**
 * dikasihnya process_manager_state
*/
struct process_manager_state {
    uint32_t active_process_count; 
    uint32_t pid;
} process_manager_state;


/**
 * i assume _process_list size is in macro PROCESS_COUNT_MAX that defined
 * in process.h
 * ini gimana sih? harus bikin di header? atau GIMANA!!?!?!?!?!?
*/
static struct ProcessControlBlock _process_list[PROCESS_COUNT_MAX];

/**
 * buat ngetrack total dari process yang ada atau dari PID
*/
static uint32_t total_process = 0;


/**
 * uh dari namanya dia generate pid baru? 
 * jadi yang dari state dijumlahin aja i guess? 
*/
uint32_t process_generate_new_pid(void){
    return process_manager_state.pid++;
}


int32_t process_create_user_process(struct FAT32DriverRequest request) {
    int32_t retcode = PROCESS_CREATE_SUCCESS; 
    if (process_manager_state.active_process_count >= PROCESS_COUNT_MAX) {
        retcode = PROCESS_CREATE_FAIL_MAX_PROCESS_EXCEEDED;
        goto exit_cleanup;
    }

    // Ensure entrypoint is not located at kernel's section at higher half
    if ((uint32_t) request.buf >= KERNEL_VIRTUAL_ADDRESS_BASE) {
        retcode = PROCESS_CREATE_FAIL_INVALID_ENTRYPOINT;
        goto exit_cleanup;
    }

    // Check whether memory is enough for the executable and additional frame for user stack
    uint32_t page_frame_count_needed = ceil_div(request.buffer_size + PAGE_FRAME_SIZE, PAGE_FRAME_SIZE);
    if (!paging_allocate_check(page_frame_count_needed) || page_frame_count_needed > PROCESS_PAGE_FRAME_COUNT_MAX) {
        retcode = PROCESS_CREATE_FAIL_NOT_ENOUGH_MEMORY;
        goto exit_cleanup;
    }

    // Process PCB 
    int32_t p_index = process_list_get_inactive_index();
    /**
     * what is this!??!?!?!?
    */
    struct ProcessControlBlock *new_pcb = &(_process_list[p_index]);

    /**
     * new_pcb huh, what is this?
    */
    new_pcb->metadata.pid = process_generate_new_pid();

exit_cleanup:
    return retcode;
}

bool process_destroy(uint32_t pid){
    for(int i = 0; i < PROCESS_COUNT_MAX; i++){
        if(_process_list[i].metadata.pid == pid){
            memset(&_process_list[i], 0, sizeof(struct ProcessControlBlock)); // gatau jir aku ngarang
            total_process--;
            return true;
        }
    }
    return false;
}