#include "cd.h"
#include "user-shell.h"




void cd(){
    char* dir = my_strtok(NULL, '/');  // Parse the next token using '/'
    uint32_t search_directory_number = state.current_directory;
    
    if(dir == NULL){
        return;  // No directory specified
    }

    if (strcmp(dir, "..", 2) == 0) {
        // Move to the parent directory
        uint32_t parent_cluster = state.curr_dir.table[0].cluster_low + ((uint32_t)state.curr_dir.table[0].cluster_high << 16);
        search_directory_number = parent_cluster;
        updateDirectoryTable(search_directory_number);
        state.current_directory = parent_cluster;
        // syscall(PUT_CHARS, (uint32_t)state.curr_dir.table->name, 8, 0);
        return;
    }

    // if (isPathAbsolute(dir, '/')) {
    //     search_directory_number = ROOT_CLUSTER_NUMBER;  // Absolute path
    // }

    if(dir != NULL) {
        updateDirectoryTable(search_directory_number);  

        char name[8];  
        copyStringWithLength(name, dir, 8);

        int entry_index = findEntryName(name);  

        // if (entry_index == -1 || state.curr_dir.table[entry_index].attribute != ATTR_SUBDIRECTORY) {
        //     syscall(6, (uint32_t) "cd: Invalid directory path", strlen("cd: Invalid directory path"), 0);
        //     syscall(5, (uint32_t) '\n', 0, 0);
        //     return;
        // }

        // Update the search_directory_number to the found directory
        if(state.curr_dir.table[entry_index].attribute == ATTR_SUBDIRECTORY){
            search_directory_number = (uint32_t)((state.curr_dir.table[entry_index].cluster_high << 16) | state.curr_dir.table[entry_index].cluster_low);
        }
        else{

        }

        dir = my_strtok(NULL, '/');  // Get the next token
    }

    // Update the current directory in the shell state
    state.current_directory = search_directory_number;
    updateDirectoryTable(state.current_directory);
    // syscall(14, (uint32_t)state.curr_dir.table->name, 0, 0);
    syscall(PUT_CHARS, (uint32_t)state.curr_dir.table->name, strlen(state.curr_dir.table->name), 0);
}