#include <stdint.h>
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"
#include "header/driver/keyboard.h"
// #include "./user-shell.h"


#define MAX_PROMPT 512 //gada perintah yang melebihi ini

struct ShellState {
	struct FAT32DirectoryTable curr_dir;
	char prompt[MAX_PROMPT];
	int prompt_size;
};

struct ShellState state = {};

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}


// // void clear() {
// // 	syscall(5, (uint32_t) '\e', 0, 0xF);
// // 	syscall(5, (uint32_t) 'J', 0, 0xF);
// // }

void ls() {
    // syscall(6, (uint32_t)"HORE", 4, 0);
	for (int i = 0; i < TOTAL_DIRECTORY_ENTRY; ++i) {
		struct FAT32DirectoryEntry *entry = &state.curr_dir.table[i];
		if (entry->user_attribute != UATTR_NOT_EMPTY) continue;
        syscall(6, (uint32_t) entry->name, strlen(entry->name), 0);
		if (entry->attribute != ATTR_SUBDIRECTORY){
            syscall(6, (uint32_t) entry->ext, strlen(entry->ext), 0);
        } 
		syscall(5, (uint32_t)' ', 0, 0);
	}
}

void run_prompt() {
    // char *token = strtok(state.prompt, ' ');

    // if (token != NULL) {
    //     if (strcmp(token, "ls", 2) == 0) {
            syscall(6, (uint32_t) "OKE", strlen("OKE"), 0);
            ls();
    //     }
    // }
    state.prompt[state.prompt_size] = '\0';
}

//buat nulis di shell
void get_prompt(){
    state.prompt_size = 0;
    while(1){
        char c = '\0'; 
        while(c == '\0'){
            syscall(10, (uint32_t) &c, 0, 0);
        }
        syscall(5, (uint32_t)c, 0, 0xF);
        if(c == '\n' || state.prompt_size + 1 >= MAX_PROMPT){
            break;
        }
        state.prompt[state.prompt_size++] = c;
    }
    state.prompt[state.prompt_size] = '\0';
}


int main(void) {
    int32_t retcode;
    struct ClusterBuffer      cl[2]   = {0};
    struct FAT32DriverRequest request = {
        .buf                   = &cl,
        .name                  = "shell",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = CLUSTER_SIZE*2,
    };
    syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);


    syscall(6, (uint32_t) "LostOnesWeeping-user", 20, 0xF);
    syscall(7, 0, 0, 0);
    while (true) {
        syscall(6, (uint32_t)"> ", 2, 0);
        get_prompt();
        run_prompt();
    }
    return 0;
}

