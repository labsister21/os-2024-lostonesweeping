#ifndef _PROCESS_H
#define _PROCESS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "header/cpu/interrupt.h"
#include "header/memory/paging.h"
#include "header/filesystem/fat32.h"

#define PROCESS_NAME_LENGTH_MAX          32
#define PROCESS_PAGE_FRAME_COUNT_MAX     8
#define PROCESS_COUNT_MAX                16

#define KERNEL_RESERVED_PAGE_FRAME_COUNT 4
#define KERNEL_VIRTUAL_ADDRESS_BASE      0xC0000000

#define CPU_EFLAGS_BASE_FLAG               0x2
#define CPU_EFLAGS_FLAG_CARRY              0x1
#define CPU_EFLAGS_FLAG_PARITY             0x4
#define CPU_EFLAGS_FLAG_AUX_CARRY          0x10
#define CPU_EFLAGS_FLAG_ZERO               0x40
#define CPU_EFLAGS_FLAG_SIGN               0x80
#define CPU_EFLAGS_FLAG_TRAP               0x100
#define CPU_EFLAGS_FLAG_INTERRUPT_ENABLE   0x200
#define CPU_EFLAGS_FLAG_DIRECTION          0x400
#define CPU_EFLAGS_FLAG_OVERFLOW           0x800
#define CPU_EFLAGS_FLAG_IO_PRIVILEGE       0x3000
#define CPU_EFLAGS_FLAG_NESTED_TASK        0x4000
#define CPU_EFLAGS_FLAG_MODE               0x8000
#define CPU_EFLAGS_FLAG_RESUME             0x10000
#define CPU_EFLAGS_FLAG_VIRTUAL_8086       0x20000
#define CPU_EFLAGS_FLAG_ALIGNMENT_CHECK    0x40000
#define CPU_EFLAGS_FLAG_VINTERRUPT_FLAG    0x80000
#define CPU_EFLAGS_FLAG_VINTERRUPT_PENDING 0x100000
#define CPU_EFLAGS_FLAG_CPUID_INSTRUCTION  0x200000
#define CPU_EFLAGS_FLAG_AES_SCHEDULE_LOAD  0x40000000
#define CPU_EFLAGS_FLAG_ALTER_INSTRUCTION  0x80000000

// Return code constant for process_create_user_process()
#define PROCESS_CREATE_SUCCESS                   0
#define PROCESS_CREATE_FAIL_MAX_PROCESS_EXCEEDED 1
#define PROCESS_CREATE_FAIL_INVALID_ENTRYPOINT   2
#define PROCESS_CREATE_FAIL_NOT_ENOUGH_MEMORY    3
#define PROCESS_CREATE_FAIL_FS_READ_FAILURE      4


/**
 * INI TERUS CONTEXT TARUH MANA BJIR? 
 * ah susah kali 
 * asumsi ini aja deh
*/

/**
 * Contain information needed for task to be able to get interrupted and resumed later
 *
 * @param cpu                         All CPU register state
 * @param eip                         CPU instruction counter to resume execution
 * @param eflags                      Flag register to load before resuming the execution
 * @param page_directory_virtual_addr CPU register CR3, containing pointer to active page directory
 */

/**
 * ini ada guidebook bagian 
 * 3.1.2.1
*/
extern struct process_state {
    uint32_t active_process_count; // Total number of active processes
    uint32_t last_pid;             // Last assigned process ID
} process_state;



struct Context {
    // TODO: Add important field here
    struct CPURegister cpu;
    uint32_t eflags; 
    uint32_t eip;
    struct PageDirectory *page_directory_virtual_addr;
    /**
     * terus cpu, epi, eflags taruh mana bjir... sumpah gahabis fikir
     * kayanya udah bjir. oke 
     * keknya ini harusnya pake InterruptFrame deh
    */
};


/**
 * EXPLAIN: keknya ProcessControlBlock ditaruh disini 
 * tau ah asumsi saya gitu.
 * so, hold up jadi ProcessControlBlock dia jadi list
 * terus di.c dia new_pcb itu _process_list dan dia punya pid? 
 * jadi dia ada struct di dalam struct!???!!?
*/

/**
 * GAMBARAN KASAR!!!!!
 * apa ini T_T
*/

//buat state untuk ambil dari process list? 
/**
 * tau ah gua ngarang bebas ini. 
*/
enum ProcessState{
    Inactive, 
    Running, 
    Waiting,
};

struct ProcessControlBlock{
    struct {
        uint32_t pid; 
        enum ProcessState state;
        // ini ada isi lagi
    } metadata;
    //i dont know why, but i think i should include Context into this struct 
    struct Context context;
    /**
     * ini memory ada di guidebook bagian 3.1.2.3
    */
    struct{
        void *virtual_addr_used[PROCESS_PAGE_FRAME_COUNT_MAX];
        uint32_t page_frame_used_count;
    } memory;
};


/**
 * i assume _process_list size is in macro PROCESS_COUNT_MAX that defined
 * in process.h
 * ini gimana sih? harus bikin di header? atau GIMANA!!?!?!?!?!?
*/
extern struct ProcessControlBlock _process_list[PROCESS_COUNT_MAX];

/**
 * Get currently running process PCB pointer
 * 
 * @return Will return NULL if there's no running process
 */
struct ProcessControlBlock* process_get_current_running_pcb_pointer(void);

/**
 * Create new user process and setup the virtual address space.
 * All available return code is defined with macro "PROCESS_CREATE_*"
 * 
 * @note          This procedure assumes no reentrancy in ISR
 * @param request Appropriate read request for the executable
 * @return        Process creation return code
 */
int32_t process_create_user_process(struct FAT32DriverRequest request);

/**
 * Destroy process then release page directory and process control block
 * 
 * @param pid Process ID to delete
 * @return    True if process destruction success
 */
bool process_destroy(uint32_t pid);

#endif