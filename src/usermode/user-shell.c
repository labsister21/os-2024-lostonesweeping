#include <stdint.h>
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"
#include "header/driver/keyboard.h"
// #include "./user-shell.h"

#define READ 0 
#define READ_DIRECTORY 1 
#define WRITE 2 
#define DELETE 3
#define PUT_CHAR 5 
#define PUT_CHARS 6 
#define ACTIVATE_KEYBOARD 7 
#define DEACTIVATE_KEYBOARD 8 
#define GET_PROMPT 10 




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


void clear() {
	syscall(PUT_CHAR, (uint32_t) '\e', 0, 0xF);
	syscall(PUT_CHAR, (uint32_t) 'J', 0, 0xF);
}

void ls() {
	for (int i = 0; i < TOTAL_DIRECTORY_ENTRY; ++i) {
		struct FAT32DirectoryEntry *entry = &state.curr_dir.table[i];
        if(entry->name != state.curr_dir.table->name){
            if (entry->user_attribute != UATTR_NOT_EMPTY) continue;
            syscall(6, (uint32_t) entry->name, strlen(entry->name), 0);
            if (entry->attribute != ATTR_SUBDIRECTORY){
                syscall(6, (uint32_t) entry->ext, strlen(entry->ext), 0);
            } 
            syscall(5, (uint32_t)' ', 0, 0);
        }
	}
}

void run_prompt() {
    char *token = my_strtok(state.prompt, ' ');
    if (token != NULL) {
        bool isClear = strcmp(token, "clear", 5) == 0;
        if(isClear) clear();
        if (strcmp(token, "ls", 2) == 0) {
            ls();
        }
        else if(strcmp(token, "mkdir", 5) == 0){
            syscall(6, (uint32_t) "OKE", strlen("OKE"), 0);
        }
        else{
            syscall(6, (uint32_t) "Gada perintahnya lmao", strlen("Gada perintahnya lmao"), 0);
        }
        if(!isClear) syscall(PUT_CHAR, (uint32_t)'\n', 0, 0xF);
        
    }


}

//buat nulis di shell
void get_prompt(){
    state.prompt_size = 0;
    while(1){
        char c = '\0'; 
        while(c == '\0'){
            syscall(GET_PROMPT, (uint32_t) &c, 0, 0);
        }
        if(c == '\b'){
            if(state.prompt_size > 0){
                state.prompt[state.prompt_size--] = c;
                syscall(12, (uint32_t)' ', 0, 0xF);
            }
        }else{
            syscall(PUT_CHAR, (uint32_t)c, 0, 0xF);
            if(c == '\n' || state.prompt_size + 1 >= MAX_PROMPT){
                break;
            }
            state.prompt[state.prompt_size++] = c;
        }
    }
    state.prompt[state.prompt_size] = '\0';
}


int main(void) {
	int8_t ret;
    int8_t ret2;
    struct FAT32DriverRequest req2;
	req2.parent_cluster_number = ROOT_CLUSTER_NUMBER;
	req2.buffer_size = 0;
	copyStringWithLength(req2.name, "dir", 8);
    syscall(WRITE, (uint32_t)&req2, (uint32_t)&ret2, 0);


	struct FAT32DriverRequest req = {
        .buf = &state.curr_dir,
        .name = "root",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER, 
        .buffer_size = 0, 
    };
    syscall(READ_DIRECTORY, (uint32_t)&req, (uint32_t)&ret, 0);

    syscall(7, 0, 0, 0);
    while (true) {
        syscall(6, (uint32_t)"LostOnesWeeping> ", 17, 0);
        get_prompt();
        run_prompt();
    }
    return 0;
}

