#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"
#include "header/driver/keyboard.h"

#define READ 0 
#define READ_DIRECTORY 1 
#define WRITE 2 
#define DELETE 3
#define PUT_CHAR 5 
#define PUT_CHARS 6 
#define ACTIVATE_KEYBOARD 7 
#define DEACTIVATE_KEYBOARD 8 
#define GET_PROMPT 10 
#define CHANGE_DIR 13


#define MAX_PROMPT 512 //gada perintah yang melebihi ini

struct ShellState {
	struct FAT32DirectoryTable curr_dir;
    uint32_t current_directory;
	char prompt[MAX_PROMPT];
	int prompt_size;
};

extern struct ShellState state;

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);


#endif
