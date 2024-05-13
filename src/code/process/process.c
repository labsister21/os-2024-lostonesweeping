#include "header/process/process.h"
#include "header/memory/paging.h"
#include "header/stdlib/string.h"
#include "header/cpu/gdt.h"







/**
 * uh dari namanya dia generate pid baru? 
 * jadi yang dari state dijumlahin aja i guess? 
*/
uint32_t process_generate_new_pid(void){
    return process_manager_state.last_pid++;
}

uint32_t get_current_pid(){
    return process_manager_state.last_pid;
}



/// gatau anjir ini bener ga sih? 
struct ProcessControlBlock* process_get_current_running_pcb_pointer(void) {
    uint32_t current_pid = get_current_pid(); 
    
    for(int i = 0; i < PROCESS_COUNT_MAX; i++){
        if(_process_list[i].metadata.state == Running){
            return &_process_list[i];
        }
    }
    
    return NULL;
}


//di bawah ada yang pake fungsi ini, tapi gadibuatin harus mikir deh lmao 
/**
 * did we need state? in struct? but how? enum?
 * karena dia di bawah dipakenya int32_t yaudah return index dari integer dia aja bjir. 
 * 
*/
int32_t process_list_get_inactive_index(){
    for(int i = 0; i < PROCESS_COUNT_MAX; i++){
        if(_process_list[i].metadata.state == Inactive) return i;
    }
    return -1;
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
    uint32_t page_frame_count_needed = (request.buffer_size + PAGE_FRAME_SIZE +  PAGE_FRAME_SIZE)/PAGE_FRAME_SIZE;
    if (!paging_allocate_check(page_frame_count_needed) || page_frame_count_needed > PROCESS_PAGE_FRAME_COUNT_MAX) {
        retcode = PROCESS_CREATE_FAIL_NOT_ENOUGH_MEMORY;
        goto exit_cleanup;
    }

    // Process PCB 
    int32_t p_index = process_list_get_inactive_index();
    struct ProcessControlBlock *new_pcb = &(_process_list[p_index]);
    /**
     * terus diapain? dia udah ngambil yang gak aktif terus? statenya jadi antri? 
     * oke, pid-nya diambil dari pid yang paling terakhir 
     * terus harusnya state dia jadi waiting
    */
    new_pcb->metadata.pid = process_generate_new_pid();
    new_pcb->metadata.state = Waiting;
    /**
     * terus ngapain bjir? apa sih ini terus ngapain WOEY!!!
    */

    //apa harus bikin paging baru? 
    /**
     * ini kayaknya buat 3.1.3. dari guidebook 
    */
   /**
    * anak anj, paging_create_new_page juga harus bikin sendiri dong!!!
   */
   struct PageDirectory *page_directory = paging_create_new_page_directory();

    new_pcb->memory.virtual_addr_user[0];
    new_pcb->memory.page_frame_used_count++;

exit_cleanup:
    return retcode;
}

bool process_destroy(uint32_t pid){
    for(int i = 0; i < PROCESS_COUNT_MAX; i++){
        if(_process_list[i].metadata.pid == pid){
            memset(&_process_list[i], 0, sizeof(struct ProcessControlBlock)); // gatau jir aku ngarang
            process_manager_state.active_process_count--;
            return true;
        }
    }
    return false;
}