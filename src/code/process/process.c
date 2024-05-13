#include "header/process/process.h"
#include "header/memory/paging.h"
#include "header/stdlib/string.h"
#include "header/cpu/gdt.h"

/**
 * ini sama ada di bagian guidebook 
 * 3.1.2.2
*/
struct process_state process_manager_state = {
    .active_process_count = 0,
    .last_pid = 0,
};

struct ProcessControlBlock _process_list[PROCESS_COUNT_MAX];
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
    // uint32_t current_pid = get_current_pid(); 
    
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
    /**
     * VALIDASI: pada bagian ini dilakukan validasi-validasi
    */
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

    /**
     * PEMBUATAN: pembuatan virtual address dengan page directory
    */
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
    * Nah, pagingnya dah bener kayanya lanjut process. 
   */
    struct PageDirectory *page_directory = paging_create_new_page_directory();
    //setelah dibikin page, kitamasukin ke ProcessControlBLOCK!!!
    new_pcb->context.page_directory_virtual_addr = page_directory;
    /**
     * bentar process kan ganti-gantian dari process satu ganti ke process lainnya 
     * perlu nyimpen current_page kah? 
    */
    struct PageDirectory *current_page = paging_get_current_page_directory_addr();

    //uh setelah dibikin terus? dipake kah?
    /**
     * hmm jadi ini kaya buat proses ganti-gantian? 
     * ini masuknya ke bab 3.1.3? 
    */
    paging_use_page_directory(page_directory);
    /**
     * habis dipake terus diapain bjir? 
     * kan param request tapi disini blm dipake apa mulai dari sini? 
    */
   /**
    * kita pake void pointer karena di paging_allocate_user_page_frame dia nerima parameter 
    * void *virtual_address
   */
    read(request);
    void *program_base_address = request.buf; //*virtual_address
    paging_allocate_user_page_frame(page_directory, program_base_address);
    paging_allocate_user_page_frame(page_directory,(void *) 0xBFFFFFFC);
   /**
    * then what? we read the request? 
    * wait, which one I have to use? read or read_dir? 
    * ah fck it assume it used read. 
   */
    /**
     * setelah request dibaca perlu balik ke page sebelumnya? 
    */
    paging_use_page_directory(current_page);
    
    //wait ini sama kayak pindah-pindah directory buat shell kah? kayanya iya. 
    new_pcb->memory.virtual_addr_used[0] = program_base_address;
    new_pcb->memory.page_frame_used_count++;

    /**
     * terus di guidebook ada 
     * eflags |= CPU_EFLAGS_BASE_FLAG | CPU_EFLAGS_FLAG_INTERRUPT_ENABLE
     * buat dimana WOEY!? buat di CPURegister kah? 
    */

   /**
    * ini gimana bikinnya bjir? gelap, w buka ni link juga bingung bjir 
    * https://course.ccs.neu.edu/cs3650/unix-xv6/HTML/S/3.html
    * 
   */
   struct CPURegister *cpu = (new_pcb->context.cpu);
   /**
    * im not sure for real di cpu register ada 
    * index(edi, esi)
    * stack(ebp, esp)
    * general(ebx, edx, ecx, eax)
    * segment(gs, fs, es, ds)
    * Nah, harus diisi apa anjir!!???
   */
    /**
     * asumsi 0 karena inisialisasi
     * ini ada di bagian 3.1.3.1 
     * Jika menggunakan linker script yang diberikan panduan User 
     * Mode untuk melakukan linking, user program akan 
     * mengasumsikan instruksi program terletak pada lokasi memory 0x0 dan seterusnya. 
    */

    //inisiasi index register
    cpu->index.edi = 0;
    cpu->index.esi = 0; 

    //inisiasi stack register 
    cpu->stack.ebp = 0; 
    cpu->stack.esp = 0;

    //inisiasi general purpose register 
    cpu->general.ebx = 0; 
    cpu->general.edx = 0; 
    cpu->general.ecx = 0; 
    cpu->general.eax = 0; 

    /**
     * segment diisi apa bjir? 
     * wait oh "Segment register seharusnya menggunakan 
     * nilai Segment Selector yang menunjuk ke GDT 
     * user data segment descriptor dan memiliki Privilege Level 3."
     * hmm, tapi yang mana? 
     * gdt user data berarti: GDT_USER_DATA_SEGMENT_SELECTOR
     * privillege berarti 0x3? 
     * terus dijumlahin? atau di or? 
     * 
    */
    uint32_t segment_val = GDT_USER_DATA_SEGMENT_SELECTOR | 0x3;
    cpu->segment.gs = segment_val;
    cpu->segment.fs = segment_val; 
    cpu->segment.es = segment_val; 
    cpu->segment.ds = segment_val;

    /**
     * masih ada yang kurang 
     * bagian eflags |= CPU_EFLAGS_BASE_FLAG | CPU_EFLAGS_FLAG_INTERRUPT_ENABLE
     * taruh dimana? apa aku salah? kalau dipikir-pikir 
     * struct InterruptFrame udah include CPURegister, Interrupt_stack
     * dia ada eip sama eflags juga apa harusnya pake itu ya? 
     * eh, tapi bukunya bilang ambil dari header? 
     * jir ada banyak, pake yang mana. eh 
     * itu eflags |= !? 
     * weh pake InterruptFrame kah? 
     * */ 

    new_pcb->context.eflags |= CPU_EFLAGS_BASE_FLAG | CPU_EFLAGS_FLAG_INTERRUPT_ENABLE; 
    /**
     * nah udah, terus sisanya diisi apa? 
     * masih ada 
     * @param eip 
     * demn dibuku bagian mana bjir ini? 
    */
    new_pcb->context.eip = 0; 

    /**
     * once again, i dont know...
     * really, i dumb af. 
    */
    process_manager_state.active_process_count++;

exit_cleanup:
    return retcode;
}

bool process_destroy(uint32_t pid){
    for(int i = 0; i < PROCESS_COUNT_MAX; i++){
        if(_process_list[i].metadata.pid == pid){
            memset(&_process_list[i], 0, sizeof(struct ProcessControlBlock)); // gatau jir aku ngarang
            _process_list[i].metadata.state = Inactive;
            process_manager_state.active_process_count--;
            return true;
        }
    }
    return false;
}