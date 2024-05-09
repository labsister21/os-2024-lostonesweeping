
#include "./mkdir.h"
#include "./user-shell.h"

void mkdir() {
	char *dir;
	dir = my_strtok(NULL, '\0');
	struct FAT32DriverRequest req = {
        .name = "\0\0\0\0\0\0\0\0",
        .ext = "\0\0\0",
        .buf = 0,
        .buffer_size = 0,
	    .parent_cluster_number = state.current_directory,
    };
    copyStringWithLength(req.name, dir, 8);
	int8_t ret;
    syscall(WRITE, (uint32_t)&req, (uint32_t)&ret, 0);
    syscall(PUT_CHAR, (uint32_t)ret + '0', 0, 0);
    syscall(PUT_CHAR, (uint32_t)' ', 0, 0);
}