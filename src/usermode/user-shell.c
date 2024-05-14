
#include "./user-shell.h"
#include "./ls.h"
#include "./mkdir.h"
#include "./cd.h"
#include "./rm.h"
#include "./find.h"
#include "cat.h"
#include "cp.h"
#include "mv.h"
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
    .current_directory_name = state.curr_dir.table->name,
};
 
void updateDirectoryTable(uint32_t cluster_number) {
    syscall(CHANGE_DIR, (uint32_t)&state.curr_dir, cluster_number, 0x1);
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
        .parent_cluster_number = state.current_directory,
    };
    memcpy(req.name, state.current_directory_name, strlen(state.current_directory_name));
    syscall(READ_DIRECTORY, (uint32_t)&req, (uint32_t)&ret, 0); 
}

void run_prompt() {
    char* token = my_strtok(state.prompt_val, ' ');
    if(memcmp(token, "ls", 2) == 0){
        ls();
    }
    else if(memcmp(token, "mkdir", 5) == 0){
        char* arg = my_strtok(NULL, '\0');
        mkdir(arg);
    }
    else if(memcmp(token, "cd", 2) == 0){
        char* arg = my_strtok(NULL, '\0'); 
        cd(arg);
    }
    else if(memcmp(token, "rm", 2) == 0){
        char* arg = my_strtok(NULL, '\0'); 
        remove(arg, state.current_directory);
    } 
    else if(memcmp(token, "cat", 3) == 0){
        char* arg = my_strtok(NULL, '\0'); 
        cat(arg, state.current_directory);
    }
    else if(memcmp(token, "cp", 2) == 0){
        char* arg1 = my_strtok(NULL, ' ');
        char* arg2 = my_strtok(NULL, '\0'); 
        cp(arg1, arg2, state.current_directory);
    }
    else if(memcmp(token, "mv", 2) == 0){
        char* arg1 = my_strtok(NULL, ' ');
        char* arg2 = my_strtok(NULL, '\0'); 
        mv(arg1, arg2, state.current_directory);
    }
    else if(memcmp(token, "find", 4) == 0){
        char* arg = my_strtok(NULL, '\0'); 
        find(arg);
    }
    else{
        put_chars("Perintah tidak ada", BIOS_RED);
        put_char('\n');
        put_chars("List perintah:", BIOS_RED);
        put_char('\n');
        put_chars("cat ", BIOS_RED);
        put_chars("find ", BIOS_RED);
        put_chars("ls ", BIOS_RED);
        put_chars("mv ", BIOS_RED);
        put_chars("cp ", BIOS_RED);
        put_chars("mkdir ", BIOS_RED);
        put_chars("cd ", BIOS_RED);
        put_chars("rm", BIOS_RED);
        put_char('\n');
    }
}

void clear_prompt() {
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
            syscall(PUT_CHAR, (uint32_t)c, BIOS_GREY, 0xF);
            if(c == '\n' || state.prompt_size + 1 >= MAX_PROMPT){
                break;
            }
            state.prompt_val[state.prompt_size++] = c;
        }
    }
    state.prompt_val[state.prompt_size] = '\0';
}

void init(){
    int8_t ret;
    struct FAT32DriverRequest req = {
        .name = "root\0\0\0\0",
        .buffer_size = 0, 
        .buf = &state.curr_dir, 
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    };
    syscall(READ_DIRECTORY, (uint32_t)&req, (uint32_t)&ret, 0);

}

int main(void){
	int8_t ret;
    state.current_directory = ROOT_CLUSTER_NUMBER;
    syscall(ACTIVATE_KEYBOARD, 0, 0, 0);

    char bufer[7] = {'a', 'k', 'u', 'g', 'i', 'l', 'a'};
    struct FAT32DriverRequest req2={
        .name = "dua",
        .ext = "txt",
        .buffer_size = 10, 
        .buf = &bufer,
        .parent_cluster_number = ROOT_CLUSTER_NUMBER
    };
    syscall(WRITE, (uint32_t)&req2, (uint32_t)&ret, 0);

    
    init();
    while (true) {
        put_chars("LostOnesWeeping", BIOS_LIGHT_BLUE);
        put_chars("-IF2230", BIOS_LIGHT_GREEN);
        put_chars(":", BIOS_LIGHT_BLUE);
        print_curr_dir(state);
        put_chars("> ", BIOS_GREY);
        get_prompt();
        run_prompt(state.prompt_val);
        refresh_dir();
    }
    return 0;
}