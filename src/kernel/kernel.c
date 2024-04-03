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

    

    char c[9000];
    for (int i=0; i<9000; i++) c[i] = (char) (i % 16);
    struct FAT32DriverRequest r3 = {
        .buf = c,
        .buffer_size = 9000,
        .ext = {'t', 'x', 'a'},
        .name = {'a', 'k', 'u', 'G', 'i', 'l', 'a', 'h'},
        .parent_cluster_number = ROOT_CLUSTER_NUMBER
    };

    write(r3);
    // char c2[5] = {'a', 'k', '-','4','7'};
    // struct FAT32DriverRequest r2 = {
    //     .buf = c2,
    //     .buffer_size = 5,
    //     .ext = {'t', 'x', 't'},
    //     .name = {'b', 'u', 'k', 'b', 'e', 'r'},
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER
    // };
    // write(r2);

    // uint8_t fo = read(r2);
    // if(fo == 0){
    //     framebuffer_place('n');
    //     framebuffer_place('n');
    //     framebuffer_place('n');

    // }
    // else{
    //     framebuffer_place('n');
    //     framebuffer_place('0');
    //     framebuffer_place('t');
    // }

    // while (true){
    //     // char c;
    //     // get_keyboard_buffer(&c);
    //     // if (c) framebuffer_write(0, col++, c, 0xF, 0);
    //     // if (c) framebuffer_place(c);
    // }
}
