
#include "./user-shell.h"
#include "./ls.h"
#include "./mkdir.h"
#include "./cd.h"
#include "./rm.h"
#include "util.h"

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}


struct ShellState state = {
    .current_directory = ROOT_CLUSTER_NUMBER, 
};


int findEntryName(char* name) {
    int result = -1;

    int i = 1;
    bool found = false;
    while (i < 64 && !found) {
        if (memcmp(state.curr_dir.table[i].name, name, 8) == 0 && 
            state.curr_dir.table[i].user_attribute == UATTR_NOT_EMPTY) {
            result = i;
            found = true;
        }
        else {
            i++;
        }
    }
    return result;
}

void updateDirectoryTable(uint32_t cluster_number) {
    syscall(CHANGE_DIR, (uint32_t)&state.curr_dir, cluster_number, 0x0);
}

void clear() {
	syscall(PUT_CHAR, (uint32_t) '\e', 0, 0xF);
	syscall(PUT_CHAR, (uint32_t) 'J', 0, 0xF);
}

void refresh_dir(){
   int8_t ret;
    struct FAT32DriverRequest req={
        .name = "\0\0\0\0\0\0\0\0",
        .buffer_size = 0, 
        .buf = &state.curr_dir,
        .parent_cluster_number = state.current_directory
    };
    memcpy(req.name, state.curr_dir.table->name, strlen(state.curr_dir.table->name));
    syscall(READ_DIRECTORY, (uint32_t)&req, (uint32_t)&ret, 0); 
}

void run_prompt() {
    char* token = my_strtok(state.prompt_val, ' ');
    if(memcmp(token, "ls", 2) == 0){
        ls();
    }
    else if(memcmp(token, "mkdir", 5) == 0){
        char* arg = my_strtok(NULL, ' ');
        mkdir(arg);
    }
    else if(memcmp(token, "cd", 2) == 0){
        char* arg = my_strtok(NULL, ' '); 
        cd(arg);
    }
}

void clear_prompt() {
    // Reset prompt buffer and size
    state.prompt_size = 0;
    memset(state.prompt_val, 0, MAX_PROMPT);
}

//buat nulis di shell
void get_prompt(){
    clear_prompt();
    while(1){
        char c = '\0'; 
        while(c == '\0'){
            syscall(GET_PROMPT, (uint32_t) &c, 0, 0);
        }
        if(c == '\b'){
            if(state.prompt_size> 0){
                state.prompt_val[state.prompt_size--] = c;
                syscall(12, (uint32_t)' ', 0, 0xF);
            }
        }else{
            syscall(PUT_CHAR, (uint32_t)c, 0, 0xF);
            if(c == '\n' || state.prompt_size + 1 >= MAX_PROMPT){
                break;
            }
            state.prompt_val[state.prompt_size++] = c;
        }
    }
    state.prompt_val[state.prompt_size] = '\0';
}


int main(void) {
	int8_t ret;
    struct FAT32DriverRequest req={
        .name = "root\0\0\0\0",
        .buffer_size = 0, 
        .buf = &state.curr_dir,
        .parent_cluster_number = ROOT_CLUSTER_NUMBER
    };

    syscall(READ_DIRECTORY, (uint32_t)&req, (uint32_t)&ret, 0);
    state.current_directory = ROOT_CLUSTER_NUMBER;
    syscall(ACTIVATE_KEYBOARD, 0, 0, 0);
       
    struct FAT32DriverRequest req2={
        .name = "LMAO",
        .buffer_size = 0, 
        .buf = &state.curr_dir,
        .parent_cluster_number = ROOT_CLUSTER_NUMBER
    };

    syscall(WRITE, (uint32_t)&req2, (uint32_t)&ret, 0);

    while (true) {
        syscall(PUT_CHARS, (uint32_t)"LostOnesWeeping:", 16, 0);
        print_curr_dir(state.path_to_print, state.current_directory);
        syscall(PUT_CHARS, (uint32_t)"> ", 2, 0);
        get_prompt();
        run_prompt(state.prompt_val);
        refresh_dir();
    }
    return 0;
}