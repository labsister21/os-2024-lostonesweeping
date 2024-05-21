#include "user-shell.h"
#include "play.h"
#include "util.h"

void delay(unsigned int milliseconds) {
    for (unsigned int i = 0; i < milliseconds; ++i) {
        // Approximate delay by executing empty loop
        for (volatile int j = 0; j < 10000; ++j) {
            // Do nothing
        }
    }
}

#define ROW_FRAME 11
#define COL_FRAME 60
#define TOTAL_CHAR_FRAME 5 + (ROW_FRAME * COL_FRAME)

void play(char* val, uint32_t curr_pos){
    if(val == NULL){
        put_chars("play: Argumen kurang", BIOS_RED); 
        put_char('\n');
        put_chars("play: play <nama_file>", BIOS_LIGHT_BLUE);
        put_char('\n');
        return;
    }
    uint32_t search_directory_number = state.current_directory;

    char directories[10][12]; 
    int num_dir; 

    extract_dir(val, directories, &num_dir);
    char true_target[12] = "\0\0\0\0\0\0\0\0\0\0\0\0";
    memcpy(true_target, directories[num_dir - 1], 12);

    char filename[8] = {'\0','\0','\0','\0','\0','\0','\0','\0'};
    char fileext[3] = {'\0','\0','\0'};
    extract_filename(true_target, filename);
    extract_file_extension(true_target, fileext);

    int i = 0;
    if(num_dir > 1){
        while (i < num_dir - 1) {
            updateDirectoryTable(search_directory_number);  

            int entry_index = findEntryName(directories[i]);  
            if (entry_index == -1 || state.curr_dir.table[entry_index].attribute != ATTR_SUBDIRECTORY) {
                put_chars("play: Invalid Directory", BIOS_RED); 
                put_char('\n');
                return;
            }

            // Update the search_directory_number to the found directory
            search_directory_number = (uint32_t)((state.curr_dir.table[entry_index].cluster_high >> 16) | state.curr_dir.table[entry_index].cluster_low);
            put_char('\n');
            i++;
        }
    } updateDirectoryTable(curr_pos);


    struct GiantClusterBuffer cl;
    struct FAT32DriverRequest request = {
        .buf = &cl,
        .name = "\0\0\0\0\0\0\0",
        .ext = "\0\0\0",
        .parent_cluster_number = search_directory_number,
        .buffer_size = 350 * CLUSTER_SIZE,
    };
    memcpy(&(request.name), filename, 8);
    memcpy(&(request.ext), fileext, 3);
    int32_t retcode;

    syscall(READ, (uint32_t) &request, (uint32_t) &retcode, 0x0);
    if(retcode == 0){
        syscall(CLEAR, 0, 0, 0);
        char* token = my_strtok((char *)&cl, '\n'); 
        int newline = 0;
        while (token != NULL) { 
            if (strlen(token) > 10){
                token[strlen(token)-1] = '\0';
                token[strlen(token)-2] = '\0';
            }
            put_chars(token, BIOS_WHITE); 
            put_chars("\n", BIOS_WHITE);
            token = my_strtok(NULL, '\n'); 
            newline++;
            if (newline == 14){
                //delay with volatile int unreliable?
                //delay(894);
                delay(814);
                syscall(CLEAR, 0, 0, 0);
                newline = 0;
            }
        } 
    }
    put_char('\n');
}