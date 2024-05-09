#include "user-shell.h"
#include "cat.h"
#include "util.h"


void cat(char* val){
    uint32_t search_directory_number = state.current_directory;
    struct ClusterBuffer cl           = {0};
    struct FAT32DriverRequest request = {
        .buf = &cl,
        .name = "\0\0\0\0\0\0\0",
        .ext = "\0\0\0",
        .parent_cluster_number = search_directory_number,
        .buffer_size = 4 * CLUSTER_SIZE,
    };

    char filename[8] = {'\0','\0','\0','\0','\0','\0','\0','\0'};
    char fileext[3] = {'\0','\0','\0'};
    
    extractBaseName(val, filename);
    extractExtension(val, fileext);

    memcpy(&(request.name), filename, 8);
    memcpy(&(request.ext), fileext, 3);
    int32_t retcode;

    syscall(READ, (uint32_t) &request, (uint32_t) &retcode, 0x0);

    if(retcode == 0){
        put_chars((char *)&cl);
    }
    put_char('\n');


}