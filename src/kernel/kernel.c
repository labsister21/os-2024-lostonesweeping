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
#include "header/memory/paging.h"


// void kernel_setup(void) {
//     load_gdt(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     activate_keyboard_interrupt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     initialize_filesystem_fat32();

//     keyboard_state_activate();
//     while (true){
//         char c;
//         get_keyboard_buffer(&c);
//         if (c) framebuffer_place(c);
//     }
// }

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    initialize_filesystem_fat32();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);

    gdt_install_tss();
    set_tss_register();

    // Allocate first 4 MiB virtual memory
    paging_allocate_user_page_frame(&_paging_kernel_page_directory, (uint8_t*) 0);

    // Write shell into memory
    // struct FAT32DriverRequest request1 = {
    //     .buf                   = (uint8_t*) 0,
    //     .name                  = "shell",
    //     .ext                   = "\0\0\0",
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    //     .buffer_size           = 0x864, 
    // };
    // write(request1);

    // uint8_t buf[4096];
    struct FAT32DriverRequest request = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0x100000,
    };
    read(request);
    // keyboard_state_activate();
    // while(true){
    //     char c;
    //     get_keyboard_buffer(&c);
    //     if(c)framebuffer_put(c);
    // }
    // Set TSS $esp pointer and jump into shell 
    // char b;
	// struct FAT32DriverRequest req = {
    //     .buf = &b,
    //     .name = "root",
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER, 
    //     .buffer_size = 0, 
    // };
    // write(req);
    // framebuffer_put('Z');
    // read_directory(req);
    // framebuffer_put('X');
    set_tss_kernel_current_stack();
    kernel_execute_user_program((uint8_t*) 0);

    while (true);
}
