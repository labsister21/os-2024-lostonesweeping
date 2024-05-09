#include "util.h"

void clearBuf(void *pointer, size_t n) {
    uint8_t *ptr       = (uint8_t*) pointer;
    for (size_t i = 0; i < n; i++) {
        ptr[i] = 0x00;
    }
}


void print_curr_dir(char* path_str, uint32_t current_dir) {
    int pathlen = 0;
    int nodecount = 0;
    char nodeIndex [10][64];

    clearBuf(path_str, 128);
    for (int i = 0; i < 10; i++) {
        clearBuf(nodeIndex[i], 64);
    }

    if (current_dir == ROOT_CLUSTER_NUMBER) {
        path_str[pathlen++] = '/';
        syscall(PUT_CHARS, (uint32_t)path_str, 1, 0);
        return;
    }
    
    // Loop sampe parentnya ROOT
    uint32_t parent = current_dir;
    path_str[pathlen++] = '/';
    while (parent != ROOT_CLUSTER_NUMBER) {
        // Isi dir_table dengan isi dari cluster sekarang
        updateDirectoryTable(parent);

        // Ambil parentnya
        parent = (uint32_t) ((state.curr_dir.table[0].cluster_high << 16) | state.curr_dir.table[0].cluster_low);
        
        // Masukin namanya ke list
        memcpy(nodeIndex[nodecount], state.curr_dir.table[0].name, strlen(state.curr_dir.table[0].name));
        nodecount++;
    }

    // Iterate back to get the full pathstr
    for (int i = nodecount - 1; i >= 0; i--) {
        for (size_t j = 0; j < strlen(nodeIndex[i]); j++) {
            path_str[pathlen++] = nodeIndex[i][j];
        } 
        
        if (i > 0) {
            path_str[pathlen++] = '/';
        }
    }

    syscall(PUT_CHARS, (uint32_t)path_str, strlen(path_str), 0);
}

void put_char(char buf){
    syscall(5,(uint32_t)buf, 0, 0);
}

void put_chars(char* buf){
    syscall(6, (uint32_t)buf, strlen(buf), 0);
}


bool isPathAbsolute(char* args_val, int (*args_info)[2], int args_pos) {
    return (memcmp(args_val + (*(args_info + args_pos))[0], "/", 1) == 0);
}