#include "./ls.h"
#include "./user-shell.h"

void ls(){
	for (int i = 0; i < TOTAL_DIRECTORY_ENTRY; ++i){
		struct FAT32DirectoryEntry *entry = &state.curr_dir.table[i];
        // if(state.current_directory == entry->cluster_low + ((uint32_t)entry->cluster_high << 16)){
            if(entry->name != state.curr_dir.table->name){
                if (entry->user_attribute != UATTR_NOT_EMPTY) continue;
                syscall(PUT_CHARS, (uint32_t) entry->name, strlen(entry->name), 0);
                if (entry->attribute != ATTR_SUBDIRECTORY){
                    if(strlen(entry->ext) != 0 ) syscall(5, (uint32_t) '.', 0, 0);
                    syscall(PUT_CHARS, (uint32_t) entry->ext, strlen(entry->ext), 0);
                } 
                syscall(PUT_CHAR, (uint32_t)' ', 0, 0);
            }
        // }
	}
}
