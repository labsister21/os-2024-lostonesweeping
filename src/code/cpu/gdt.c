#include "../../header/cpu/gdt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            .segment_low = 0,
            .segment_high = 0,
            .base_low = 0, 
            .base_mid = 0, 
            .base_high = 0,
            .type_bit = 0,
            .non_system = 0,
            .privillege_bit = 0,
            .present_bit = 0,
            .code_segment_bit = 0, 
            .default_bit = 0,
            .granularity_bit = 0,
            .available_bit = 0

        },
        {
            .segment_low = 0xFFFF, 
            .segment_high = 0xF, 
            .base_low = 0, 
            .base_mid = 0, 
            .base_high = 0,
            .type_bit = 0xA, 
            .non_system = 1, 
            .privillege_bit = 0, 
            .present_bit = 1, 
            .code_segment_bit = 0, 
            .default_bit = 1,
            .granularity_bit = 1,
            .available_bit = 0
        },
        {
            .segment_low = 0xFFFF,
            .segment_high = 0xF,
            .base_low = 0, 
            .base_mid = 0,
            .base_high = 0,
            .type_bit = 0x2, 
            .non_system = 1, 
            .privillege_bit = 0, 
            .present_bit = 1, 
            .code_segment_bit = 0, 
            .default_bit= 1, 
            .granularity_bit = 1,
            .available_bit = 0
        }
    }
};

/**
 * _gdt_gdtr, predefined system GDTR. 
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    // TODO : Implement, this GDTR will point to global_descriptor_table. 
    //        Use sizeof operator
    .size = sizeof(global_descriptor_table) - 1,
    .address = &global_descriptor_table
};
