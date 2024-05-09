#include "cd.h"
#include "user-shell.h"


void cd(char* dir) {
    uint32_t search_directory_number = state.current_directory;

    if (dir == NULL) {
        return;  // No directory specified
    }

    char* token = my_strtok(dir, '/');  // Tokenize the path
    while (token != NULL) {
        updateDirectoryTable(search_directory_number);  

        char name[8];
        copyStringWithLength(name, token, 8);

        int entry_index = findEntryName(name);  

        if (entry_index == -1 || state.curr_dir.table[entry_index].attribute != ATTR_SUBDIRECTORY) {
            syscall(6, (uint32_t) "cd: Invalid directory path", strlen("cd: Invalid directory path"), 0);
            syscall(5, (uint32_t) '\n', 0, 0);
            return;
        }

        // Update the search_directory_number to the found directory
        search_directory_number = (uint32_t)((state.curr_dir.table[entry_index].cluster_high << 16) | state.curr_dir.table[entry_index].cluster_low);

        token = my_strtok(NULL, '/');  // Get the next token
    }

    // Update the current directory in the shell state
    state.current_directory = search_directory_number;
    updateDirectoryTable(state.current_directory);

    // Print the current directory name to the shell
    syscall(PUT_CHARS, (uint32_t)state.curr_dir.table->name, strlen(state.curr_dir.table->name), 0);
}
