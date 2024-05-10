#include "./rm.h"
#include "./user-shell.h"
#include "./util.h"





void remove(char* target){
    uint32_t search_directory_number = state.current_directory;
    if (target == NULL) {
        return;  
    }

    char directories[10][12]; 
    int num_dir; 

    extract_dir_special(target, directories, &num_dir);
    char true_target[12] = "\0\0\0\0\0\0\0\0\0\0\0\0"; 
    memcpy(true_target, directories[num_dir - 1], 12);


    char name[8]; 
    char ext[3]; 
    extractBaseName(true_target, name);
    extractExtension(true_target, ext);


    int i = 0;
    if(num_dir > 1){
        while (i < num_dir - 1) {
            updateDirectoryTable(search_directory_number);  

            int entry_index = findEntryName(directories[i]);  
            if (entry_index == -1 || state.curr_dir.table[entry_index].attribute != ATTR_SUBDIRECTORY) {
                syscall(6, (uint32_t) "cd: Invalid directory path", strlen("cd: Invalid directory path"), 0);
                syscall(5, (uint32_t) '\n', 0, 0);
                return;
            }

            // Update the search_directory_number to the found directory
            search_directory_number = (uint32_t)((state.curr_dir.table[entry_index].cluster_high >> 16) | state.curr_dir.table[entry_index].cluster_low);
            put_char('\n');
            i++;
        }
    }

    struct FAT32DriverRequest req = {
        .buf = 0,
        .name = "\0\0\0\0\0\0\0\0",
        .ext = "\0\0\0",
	    .parent_cluster_number = search_directory_number,
        .buffer_size = 0,
        
    };
    memcpy(req.name, name, 8);
    memcpy(req.ext, ext, 3);
    
    // syscall(PUT_CHARS, (uint32_t)req.name, strlen(req.name), 0);
    // syscall(PUT_CHARS, (uint32_t)req.ext, strlen(req.ext), 0);
    int8_t ret;
    syscall(DELETE, (uint32_t)&req, (uint32_t)&ret, 0); 
}
