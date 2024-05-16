#include "../../header/cpu/portio.h"
#include "../../header/cpu/interrupt.h"
#include "../../header/driver/keyboard.h"
#include "../../header/text/framebuffer.h"
#include "../../header/filesystem/fat32.h"
#include "../../header/cpu/gdt.h"
#include "../../header/text/terminaltext.h"
#include "../../header/process/scheduler.h"

void activate_keyboard_interrupt(void) {
    out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_KEYBOARD));
}

void io_wait(void) {
    out(0x80, 0);
}

void pic_ack(uint8_t irq) {
    if (irq >= 8) out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void) {
    // Starts the initialization sequence in cascade mode
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); 
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
    io_wait();
    out(PIC1_DATA, 0b0100); // ICW3: tell Master PIC, slave PIC at IRQ2 (0000 0100)
    io_wait();
    out(PIC2_DATA, 0b0010); // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();

    // Disable all interrupts
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK);
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

/**'
 * case 0: read 
 * case 1: read directory
 * case 2: write 
 * case 3: delete 
 * case 4: get_char 
 * case 5: put_char
 * case 6: framebuffer_put_char 
 * case 7: framebuffer_put_chars
 * case 8: 
 * case 9: framebuffer_clear 
 * case 10: framebuffer_cursor 
 * case 11: time
*/
void syscall(struct InterruptFrame frame) {
    switch (frame.cpu.general.eax) {
        case 0: //READ
            *((int8_t*) frame.cpu.general.ecx) = read(
                *(struct FAT32DriverRequest*) frame.cpu.general.ebx
            );
            break;
        case 1: //READ Directory 
            *((int8_t *)frame.cpu.general.ecx) =  read_directory(*(struct FAT32DriverRequest*)frame.cpu.general.ebx);
            break;

        case 2: //write 
            *((int8_t *)frame.cpu.general.ecx) = write(*(struct FAT32DriverRequest *)frame.cpu.general.ebx);
            break;
        case 3: //delete
            *((int8_t *)frame.cpu.general.ecx) = delete(*(struct FAT32DriverRequest *)frame.cpu.general.ebx);
		    break;
        case 4: 
        /**
         * TODO:
        */
            get_keyboard_buffer((char*) frame.cpu.general.ebx);
            break;
        case 5: //char put
            if((char) frame.cpu.general.ebx){
                fputc((char)frame.cpu.general.ebx, (uint8_t) frame.cpu.general.edx);
                // framebuffer_put((char) frame.cpu.general.ebx); 
            }
            break;
        case 6: { //chars puts
            int i = frame.cpu.general.ecx;
		    char *str = (char *)frame.cpu.general.ebx;
            uint8_t color = frame.cpu.general.edx;
		    for(int j = 0; j < i; j++){
                framebuffer_put(str[j], color);
            }
        }
            break;
        case 7: 
            keyboard_state_activate();
            break;
        case 8: //write clock
            if((char)frame.cpu.general.ebx){
                int pos = frame.cpu.general.ecx;
                framebuffer_write(20, pos, (char)frame.cpu.general.ebx, 0b1010, 0);
            }
            break;
        case 10: //get_prompt
            char *ptr= (char*) frame.cpu.general.ebx; 
            get_keyboard_buffer(ptr);
            break;
        case 11: //prevent deleting? 
            int i = 0;
            char *str = (char *)frame.cpu.general.ebx;
            uint8_t color = frame.cpu.general.edx;
            while (str[i] != '\0') {
                if(str[i] != '\b'){
                    framebuffer_put(str[i], color);
                    ++i;
                }else{
                    framebuffer_clear_delete();
                }
            }
            break;
        case 12: 
            framebuffer_clear_delete();
            break;
        case 13: 
            read_clusters((struct FAT32DirectoryTable*)frame.cpu.general.ebx, frame.cpu.general.ecx, 1);
            break;
        case 14: //Terminate (frame?) process 
            struct Context ctx;
            ctx.cpu = frame.cpu;
            ctx.eflags = frame.int_stack.eflags;
            ctx.eip = frame.int_stack.eip;
            ctx.page_directory_virtual_addr = process_get_current_running_pcb_pointer()->context.page_directory_virtual_addr;
            process_get_current_running_pcb_pointer()->metadata.state = Inactive;
            scheduler_save_context_to_current_running_pcb(ctx);
            // pic_ack(IRQ_TIMER);
            // scheduler_switch_to_next_process();        
            break;
        case 15: //Pembuatan user process baru
            *((int8_t *)frame.cpu.general.ecx) =  process_create_user_process(*(struct FAT32DriverRequest*)frame.cpu.general.ebx);
            break;
        case 16: //Melakukan terminasi process berdasarkan PID
            uint32_t pid = frame.cpu.general.ebx;
            *((int8_t *)frame.cpu.general.ecx) = process_destroy(pid) ? 0 : -1; 
            break;

        case 17: // Gather and print process information
            struct ProcessInfo *info = (struct ProcessInfo *) frame.cpu.general.ebx;
            int max_count = frame.cpu.general.ecx;
            int count = 0;
            char *pid_info = "PID: ";
            char *state_info = "State: ";
            // Gather information
            for (int i = 0; i < PROCESS_COUNT_MAX && count < max_count; i++) {
                if (_process_list[i].metadata.state != Inactive) {
                    info[count].pid = _process_list[i].metadata.pid;
                    info[count].state = _process_list[i].metadata.state;

                    // Print PID info
                    for (int j = 0; j < 4; j++) {
                        framebuffer_put(pid_info[j], 0b1010);
                    }
                    framebuffer_put(info[count].pid + '0', 0b1010);
                    framebuffer_put(' ', 0b1010);

                    // Print state info
                    for (int j = 0; j < 6; j++) { 
                        framebuffer_put(state_info[j], 0b1010);
                    }
                    const char* state_str = getStateString(info[count].state);
                    for(int j = 0; state_str[j] != '\0'; j++) {
                        framebuffer_put(state_str[j], 0b1010);
                    }
                    framebuffer_newline();

                    count++;
                }
            }
            framebuffer_newline();

            frame.cpu.general.edx = count;  
            break;
        }
}

void main_interrupt_handler(struct InterruptFrame frame) {
    switch (frame.int_number) {
        case IRQ_KEYBOARD + PIC1_OFFSET:
            keyboard_isr();
            break;
        case SYSCALL_CALL: 
            syscall(frame);
            break;
        case IRQ_TIMER + PIC1_OFFSET:
            struct Context ctx;
            ctx.cpu = frame.cpu;
            ctx.eflags = frame.int_stack.eflags;
            ctx.eip = frame.int_stack.eip;
            ctx.page_directory_virtual_addr = process_get_current_running_pcb_pointer()->context.page_directory_virtual_addr;
            scheduler_save_context_to_current_running_pcb(ctx);
            pic_ack(IRQ_TIMER);
            scheduler_switch_to_next_process();
            break;
    }
}

struct TSSEntry _interrupt_tss_entry = {
    .ss0  = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
};

void set_tss_kernel_current_stack(void) {
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile ("mov %%ebp, %0": "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8; 
}
