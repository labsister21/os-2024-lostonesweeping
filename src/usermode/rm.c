#include "./rm.h"
#include "./user-shell.h"

void extractBaseName(const char *filename, char *basename) {
    int j;
    int len = strlen(filename);
    for (j = 0; j < len; j++) {
        if(filename[j] != '.') basename[j] = filename[j];
        else break;
    }
    basename[j] = '\0';
}

void extractExtension(const char *filename, char *extension) {
    int i, j;
    int len = strlen(filename);
    int dot_found = -1;

    for (i = len - 1; i >= 0; i--) {
        if (filename[i] == '.') {
            dot_found = i;
            break;
        }
    }

    int z = 0;
    if (dot_found != -1) {
        for (j = i + 1; j < len; j++) {
            extension[z++] = filename[j];
        }
    } else {
        extension[0] = '\0';
    }
}

void rm(){
    /**
     * TODO: INI BELUM REMOVE PAKE PATH dahlah gelap
    */
    char *dir; 
    char name[8]; 
    char ext[3]; 

    dir = my_strtok(NULL, '\0'); 
    	struct FAT32DriverRequest req = {
        .name = {0},
        .ext = {0},
        .buf = NULL,
        .buffer_size = 0,
	    .parent_cluster_number = (state.curr_dir.table[0].cluster_low) + (((uint32_t) state.curr_dir.table[0].cluster_high) >> 16),
        
    };
    extractBaseName(dir, name);
    extractExtension(dir, ext);

    copyStringWithLength(req.name, name, 8);
    copyStringWithLength(req.ext, ext, 3);
    
    // syscall(PUT_CHARS, (uint32_t)req.name, strlen(req.name), 0);
    // syscall(PUT_CHARS, (uint32_t)req.ext, strlen(req.ext), 0);
    int8_t ret;
    syscall(DELETE, (uint32_t)&req, (uint32_t)&ret, 0); 
}
