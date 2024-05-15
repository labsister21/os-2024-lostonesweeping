#include "ps.h"
#include "header/process/process.h"
#include "user-shell.h"

void ps(){
    struct ProcessInfo process_info; 
    syscall(17, (uint32_t)&process_info, (uint32_t)PROCESS_COUNT_MAX, 0);
    
}