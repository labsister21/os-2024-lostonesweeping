#include "kill.h"


void kill(char* pid){

    struct ProcessInfo *list_process;
    syscall(LIST, (uint32_t)&list_process, (uint32_t)PROCESS_COUNT_MAX, 0);
    
    int retval;
    for(int i = 0; i < PROCESS_COUNT_MAX; i++){
        if(list_process[i].pid == (uint32_t)pid){
            syscall(KILL, (uint32_t)&list_process[i].pid, (uint32_t)&retval, 0);
        }
    }

}