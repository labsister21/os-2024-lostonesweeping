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
#define CHANGE_DIR 13




#define MAX_PROMPT 512 //gada perintah yang melebihi ini

struct ShellState {
    uint32_t current_directory;
	struct FAT32DirectoryTable curr_dir;
	char prompt[MAX_PROMPT];
	int prompt_size;
};

struct ShellState state = {
    .current_directory = ROOT_CLUSTER_NUMBER,
};

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

bool isPathAbsolute(char *args_val, char delim) {
    // Extract the first token from args_val using my_strtok
    char *token = my_strtok(args_val, delim);

    // Check if the extracted token is not NULL and represents an absolute path ("/")
    if (token != NULL && strcmp(token, "/", 1) == 0) {
        return true; // It's an absolute path
    } else {
        return false; // It's not an absolute path
    }
}

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

    // Find the position of the last '.' character in the filename
    for (i = len - 1; i >= 0; i--) {
        if (filename[i] == '.') {
            dot_found = i;
            break;
        }
    }

    int z = 0;
    if (dot_found != -1) {
        // Copy characters from the position after '.' to the end of the string
        for (j = i + 1; j < len; j++) {
            extension[z++] = filename[j];
        }
    } else {
        // If no '.' found, return an empty string for extension
        extension[0] = '\0';
    }
}





void clear() {
	syscall(PUT_CHAR, (uint32_t) '\e', 0, 0xF);
	syscall(PUT_CHAR, (uint32_t) 'J', 0, 0xF);
}

void refresh_dir() {
	struct FAT32DriverRequest req = {
        .name = {0},
        .buf = &state.curr_dir,
        .buffer_size = 0,
        .parent_cluster_number = state.current_directory,
    };
	int8_t ret;
    copyStringWithLength(req.name, state.curr_dir.table->name, 8);
	syscall(READ_DIRECTORY, (uint32_t)&req, (uint32_t)&ret, 0);
}

void cd() {
    char* dir = my_strtok(NULL, '/');  // Parse the next token using '/'
    uint32_t search_directory_number = state.current_directory;

    if (dir == NULL) {
        return;
    }

    if (isPathAbsolute(dir, '/')) {
        search_directory_number = ROOT_CLUSTER_NUMBER;  
    }

    while (dir != NULL) {
        updateDirectoryTable(search_directory_number);  

        char name[8];  
        copyStringWithLength(name, dir, 8);

        int entry_index = findEntryName(name);  

        if (entry_index == -1 || state.curr_dir.table[entry_index].attribute != ATTR_SUBDIRECTORY) {
            syscall(6, (uint32_t) "cd: Invalid directory path", strlen("cd: Invalid directory path"), 0);
            syscall(5, (uint32_t) '\n', 0, 0);
            return;
        }

        // Update the search_directory_number to the found directory
        search_directory_number = (uint32_t)((state.curr_dir.table[entry_index].cluster_high << 16) | state.curr_dir.table[entry_index].cluster_low);

        dir = my_strtok(NULL, '/');  // Get the next token
    }

    // Update the current directory in the shell state
    state.current_directory = search_directory_number;
    updateDirectoryTable(state.current_directory);
    syscall(PUT_CHAR, (uint32_t)state.current_directory + '0', 0, 0);
    syscall(PUT_CHARS, (uint32_t)state.curr_dir.table->name, 8, 0);
}



void ls() {
	for (int i = 0; i < TOTAL_DIRECTORY_ENTRY; ++i) {
		struct FAT32DirectoryEntry *entry = &state.curr_dir.table[i];
        if(entry->name != state.curr_dir.table->name){
            if (entry->user_attribute != UATTR_NOT_EMPTY) continue;
            syscall(6, (uint32_t) entry->name, strlen(entry->name), 0);
            if (entry->attribute != ATTR_SUBDIRECTORY){
                if(strlen(entry->ext) != 0 ) syscall(5, (uint32_t) '.', 0, 0);
                syscall(6, (uint32_t) entry->ext, strlen(entry->ext), 0);
            } 
            syscall(5, (uint32_t)' ', 0, 0);
        }
	}
}

void mkdir() {
	char *dir;
	dir = my_strtok(NULL, '\0');
    syscall(PUT_CHAR, (uint32_t)state.current_directory + '0', 0, 0);
    syscall(PUT_CHAR, (uint32_t)'\n', 0, 0);
	struct FAT32DriverRequest req = {
        .name = {0},
        .buf = NULL,
        .buffer_size = 0,
	    .parent_cluster_number = state.current_directory,
    };
    copyStringWithLength(req.name, dir, 8);
	int8_t ret;
    syscall(WRITE, (uint32_t)&req, (uint32_t)&ret, 0);
    syscall(PUT_CHAR, (uint32_t)(ret + '0'), 0, 0);
    syscall(PUT_CHAR, (uint32_t)'\n', 0, 0);
    syscall(PUT_CHAR, (uint32_t)state.current_directory + '0', 0, 0);
    syscall(PUT_CHAR, (uint32_t)'\n', 0, 0);
	refresh_dir();
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
    refresh_dir();
}



void test() {
    char *c1 = my_strtok(NULL, ' ');  // Get the first token
    char *c2 = my_strtok(NULL, ' ');  // Get the second token
    char *c3 = my_strtok(NULL, '\0');  // Get the second token

    if (c1 != NULL && c2 != NULL && c3 != NULL) {
        // Print the first token
        syscall(PUT_CHARS, (uint32_t)c1, strlen(c1), 0);
        syscall(PUT_CHAR, (uint32_t)' ', 0, 0);  // Print a space between tokens

        // Print the second token
        syscall(PUT_CHARS, (uint32_t)c2, strlen(c2), 0);
        syscall(PUT_CHAR, (uint32_t)' ', 0, 0);  // Print a newline

        syscall(PUT_CHARS, (uint32_t)c3, strlen(c3), 0);
        syscall(PUT_CHAR, (uint32_t)'\n', 0, 0);  // Print a newline
    } else {
        syscall(6, (uint32_t)"Invalid usage of 'test' command", 30, 0);
        syscall(PUT_CHAR, (uint32_t)'\n', 0, 0);  // Print a newline
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
            // syscall(6, (uint32_t) "OKE", strlen("OKE"), 0);
            mkdir();
        }
        else if(strcmp(token, "rm", 2) == 0){
            rm();
        }
        else if(strcmp(token, "test", 4) == 0){
            test();
        } 
        else if(strcmp(token, "cd", 2) == 0){
            cd();
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
    char a[10];
    for(int i = 0; i < 20; i++){
        a[i] = 'A';
    }
    struct FAT32DriverRequest req2;
	req2.parent_cluster_number = ROOT_CLUSTER_NUMBER;
	req2.buffer_size = 0;
	copyStringWithLength(req2.name, "dir", 8);
    syscall(WRITE, (uint32_t)&req2, (uint32_t)&ret2, 0);

    
    struct FAT32DriverRequest reqfile = {
        .buf = &a,
        .name = "mbuh",
        .ext = "txt",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER, 
        .buffer_size = 20, 
    };
    syscall(WRITE, (uint32_t)&reqfile, (uint32_t)&ret2, 0);


    struct FAT32DriverRequest req = {
        .buf = &state.curr_dir,
        .name = "root",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER, 
        .buffer_size = 0, 
    };

    syscall(READ_DIRECTORY, (uint32_t)&req, (uint32_t)&ret, 0);

    syscall(ACTIVATE_KEYBOARD, 0, 0, 0);
    while (true) {
        syscall(PUT_CHARS, (uint32_t)"LostOnesWeeping:", 16, 0);
        syscall(PUT_CHARS, (uint32_t)state.curr_dir.table->name, strlen(state.curr_dir.table->name), 0);
        syscall(PUT_CHARS, (uint32_t)"> ", 2, 0);
        get_prompt();
        run_prompt();
    }
    return 0;
}

