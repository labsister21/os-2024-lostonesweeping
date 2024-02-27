#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"

void protectedMode(void){
    uint32_t a; 
    __asm__("cli");
    __asm__("lgdt %[gdtr]" : : [gdtr] "m"(_gdt_gdtr));
    __asm__(
        "mov %%cr0, %0;"
        "or $0x1, %0;"
        "mov %0, %%cr0;"
        : "=r"(a)
    );
    __asm__("ljmp $0x08, $PModeMain");

    __asm__(
        "PModeMain:;"
    );
}

void kernel_setup(void) {
    // uint32_t a;
    // uint32_t volatile b = 0x0000BABE;
    // __asm__("mov $0xCAFE0000, %0" : "=r"(a));

    load_gdt(&_gdt_gdtr);
    int a =0;
    protectedMode();


    while (true) a += 1;
}
