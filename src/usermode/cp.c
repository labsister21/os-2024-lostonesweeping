#include "user-shell.h"
#include "util.h"
#include "cp.h"

void cp(char* src, char* dest){
    uint32_t search_source_number = state.current_directory;
    uint32_t search_target_number = state.current_directory; 

    char directories_src[10][12]; 
    char directories_target[10][12]; 

    int num_dir_src; 
    int num_dir_target; 

    extract_dir_special(src, num_dir_src, &num_dir_src); 
    extract_dir_special(dest, num_dir_target, &num_dir_target);
}