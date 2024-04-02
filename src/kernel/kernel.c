#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/driver/keyboard.h"
#include "header/driver/disk.h"
#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"


void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();

    char c[5] = {'a', 'k', 'u','O','X'};
    // char c2[5];
    struct FAT32DriverRequest r = {
        .buf = c, 
        .buffer_size = 5,
        .ext = {'p', 'd', 'f'},
        .name = {'a', 'k', 'u', 'g', 'i', 'l', 'a'},
        .parent_cluster_number = ROOT_CLUSTER_NUMBER
    };

    write(r);
    struct FAT32DriverRequest r2 = {
        .buffer_size = 5,
        .ext = {'p', 'd', 'f'},
        .name = {'a', 'k', 'u', 'g', 'i', 'l', 'a'},
        .parent_cluster_number = ROOT_CLUSTER_NUMBER
    };
    read(r2);
    while (true){
        // char c;
        // get_keyboard_buffer(&c);
        // if (c) framebuffer_write(0, col++, c, 0xF, 0);
        // if (c) framebuffer_place(c);
    }
}
