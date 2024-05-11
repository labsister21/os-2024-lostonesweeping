#include "mv.h"
#include "user-shell.h"
#include "util.h"


void rename(char* target_name, char* target_ext, char directories_src[][12], int directories_src_num ,char directories_dest[][12], uint32_t curr_pos){
    uint32_t search_source_number = state.current_directory;


    int i = 0;
    if(directories_src_num > 1){
        while (i < directories_src_num - 1) {
            updateDirectoryTable(search_source_number);  

            int entry_index = findEntryName(directories_src[i]);  
            if (entry_index == -1 || state.curr_dir.table[entry_index].attribute != ATTR_SUBDIRECTORY) {
                syscall(6, (uint32_t) "cd: Invalid directory path", strlen("cd: Invalid directory path"), 0);
                syscall(5, (uint32_t) '\n', 0, 0);
                return;
            }

            // Update the search_directory_number to the found directory
            search_source_number = (uint32_t)((state.curr_dir.table[entry_index].cluster_high >> 16) | state.curr_dir.table[entry_index].cluster_low);
            put_char('\n');
            i++;
        }
    } updateDirectoryTable(curr_pos);

    char what_to_rename[12] = "\0\0\0\0\0\0\0\0\0\0\0\0"; 
    char filename_rename[8] = "\0\0\0\0\0\0\0\0"; 
    char fileext_rename[3] = "\0\0\0";

    memcpy(what_to_rename, directories_dest[0], 12); 
    extract_filename(what_to_rename, filename_rename); 
    extract_file_extension(what_to_rename, fileext_rename);

    struct ClusterBuffer cl = {0};
    int retcode;
    if(strlen(fileext_rename) != 0){
        /**
         * ini kasus untuk merubah nama file
        */
        struct FAT32DriverRequest request_read = {
            .buf = &cl,
            .name = "\0\0\0\0\0\0\0",
            .ext = "\0\0\0",
            .parent_cluster_number = search_source_number,
            .buffer_size = CLUSTER_SIZE,
        };
        memcpy(request_read.name, target_name, 8); 
        memcpy(request_read.ext, target_ext, 3); 
        syscall(READ, (uint32_t)&request_read, (uint32_t)&retcode, 0x0);

        struct FAT32DriverRequest request_write = {
            .buf = &cl,
            .name = "\0\0\0\0\0\0\0",
            .ext = "\0\0\0",
            .parent_cluster_number = search_source_number,
            .buffer_size = CLUSTER_SIZE,
        };  

        memcpy(request_write.name, filename_rename, 8); 
        memcpy(request_write.ext, fileext_rename, 3); 
        syscall(WRITE, (uint32_t)&request_write, (uint32_t)&retcode, 0x0);

        struct FAT32DriverRequest request_delete = {
            .name = "\0\0\0\0\0\0\0",
            .ext = "\0\0\0",
            .buf = &cl,
            .parent_cluster_number = search_source_number,
            .buffer_size = 0,
        }; 

        memcpy(request_delete.name, target_name, 8); 
        memcpy(request_delete.ext, target_ext, 3);
        syscall(DELETE, (uint32_t)&request_delete, (uint32_t)&retcode, 0x0);

        if(retcode == 0){
            put_chars("File berhasil di-rename");
            put_char('\n');
        } 
    }
    else{ 
        /**
         * ini kasus buat renaming folder;
        */
        struct FAT32DriverRequest request = {
            .buf = &cl,
            .name = "\0\0\0\0\0\0\0",
            .ext = "\0\0\0",
            .parent_cluster_number = search_source_number,
            .buffer_size = 0,
        };
        memcpy(request.name, target_name, 8); 
        syscall(READ, (uint32_t)&request, (uint32_t)&retcode, 0x0);
        syscall(DELETE, (uint32_t)&request, (uint32_t)&retcode, 0x0);
        memcpy(request.name, filename_rename, 8); 

        syscall(WRITE, (uint32_t)&request, (uint32_t)&retcode, 0x0);
        if(retcode == 0){
            put_chars("Folder berhasil di-rename");
        } 
    }
}

// void move(char* target_name, char* target_ext, char directories_src[10][12], char directories_dest[10][12]){
//     uint32_t search_source_number = state.current_directory;
//     uint32_t search_target_number = state.current_directory; 

// }


void mv(char* arg1, char* arg2, uint32_t curr_pos){

    char directories_src[10][12]; 
    char directories_target[10][12]; 

    int num_dir_src; 
    int num_dir_target; 

    extract_dir(arg1, directories_src, &num_dir_src); 
    extract_dir(arg2, directories_target, &num_dir_target);

    char target[12] = "\0\0\0\0\0\0\0\0\0\0\0\0";

    memcpy(target, directories_src[num_dir_src - 1], 12); 
    // put_chars(directories_src[num_dir_src - 1]);

    char target_name[8]; 
    char target_ext[3]; 
    extract_filename(target, target_name); 
    extract_file_extension(target, target_ext);
    
    char rename_ext[3]; 
    extract_file_extension(directories_target[num_dir_target-1], rename_ext);

    bool file;
    if(strlen(target_ext) != 0){
        file = true; 
    }else file = false;

    bool renaming;
    if(file && (num_dir_target == 1) && strlen(rename_ext) != 0){
        renaming = true; //renaming file
    }
    if(!file && (num_dir_target == 1) && strlen(rename_ext) == 0){
        renaming = true; //renaming folderk
    }
    if((file || !file) && (num_dir_target != 1)){
        renaming = false; //this_is_MOVE
    }

    if((file && renaming) || (!file && renaming)){
        rename(target_name, target_ext, directories_src, num_dir_src ,directories_target, curr_pos);
    }
    // else if((file && !renaming) || (!file && !renaming)){
    //     move(target_name, target_ext, directories_src, directories_target);
    // }
}

