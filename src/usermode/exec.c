#include "exec.h"
#include "header/process/process.h"
#include "user-shell.h"

void exec(char* arg){

    int retcode;
    struct FAT32DriverRequest request = {
        .buf                   = (uint8_t*) 0,
        .name                  = {0},
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0x100000,
    };
    memcpy(request.name, arg, 8);
    syscall(15, (uint32_t)&request, (uint32_t)&retcode, 0);
}