#include "kill.h"
#include "util.h"
#include "user-shell.h"


void kill(char* arg){

    struct ProcessInfo list_process[PROCESS_COUNT_MAX];
    int total;
    syscall(17, (uint32_t)&list_process, (uint32_t)&total, 0);

    // put_chars(arg, BIOS_LIGHT_BLUE);
    char process_name[8]; 
    memcpy(process_name, arg, 8);


    if(memcmp(process_name, "shell", 5) == 0){
        put_chars("kill: jangan dihapus dong program utama", BIOS_RED);
        put_char('\n');
        return;
    }


    int retval;
    for(int i = 0; i < total; i++){
        if(memcmp(list_process[i].name, process_name, 8) == 0){
            syscall(KILL, list_process[i].pid, (uint32_t)&retval, 0);
        }
    }

    syscall(CLEAR, 0, 0, 0);
    
    if(retval == 0){
        put_chars(arg, BIOS_RED);
        put_char('\n');
        put_chars("Berhasil di-terminate", BIOS_RED);
        put_char('\n');
    }
}