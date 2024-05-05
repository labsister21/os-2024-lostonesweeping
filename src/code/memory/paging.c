#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../header/memory/paging.h"

__attribute__((aligned(0x1000))) struct PageDirectory _paging_kernel_page_directory = {
    .table = {
        [0] = {
            .flag.present_bit       = 1,
            .flag.write_bit         = 1,
            .flag.use_pagesize_4_mb = 1,
            .lower_address          = 0,
        },
        [0x300] = {
            .flag.present_bit       = 1,
            .flag.write_bit         = 1,
            .flag.use_pagesize_4_mb = 1,
            .lower_address          = 0,
        },
    }
};

static struct PageManagerState page_manager_state = {
    .page_frame_map = {[0 ... PAGE_FRAME_MAX_COUNT-1] = false},
    .free_page_frame_count = PAGE_FRAME_MAX_COUNT
};

void update_page_directory_entry(
    struct PageDirectory *page_dir,
    void *physical_addr, 
    void *virtual_addr, 
    struct PageDirectoryEntryFlag flag
) {
    uint32_t page_index = ((uint32_t) virtual_addr >> 22) & 0x3FF;
    page_dir->table[page_index].flag          = flag;
    page_dir->table[page_index].lower_address = ((uint32_t) physical_addr >> 22) & 0x3FF;
    flush_single_tlb(virtual_addr);
}

void flush_single_tlb(void *virtual_addr) {
    asm volatile("invlpg (%0)" : /* <Empty> */ : "b"(virtual_addr): "memory");
}



/* --- Memory Management --- */
// TODO: Implement
bool paging_allocate_check(uint32_t amount) {
    // Check whether requested amount is available
    return page_manager_state.free_page_frame_count >= amount;
}


bool paging_allocate_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr) {
    // Find free physical frame and map virtual frame into it
    if(page_manager_state.free_page_frame_count == 0){
        return false;
    }

    for (uint32_t i = 0; i < PAGE_ENTRY_COUNT; ++i) {
        if (!page_manager_state.page_frame_map[i]) {
            page_manager_state.page_frame_map[i] = true;
            --page_manager_state.free_page_frame_count;
            void *physical_addr = (void *)(i * PAGE_FRAME_SIZE);
            struct PageDirectoryEntryFlag flag = {
                .present_bit = 1,
                .write_bit = 1,
                .use_pagesize_4_mb = 1,
                .user_supervisor = 1,
            };
            update_page_directory_entry(page_dir, physical_addr, virtual_addr, flag);
            return true;
        }
    }
    return false;
}

bool paging_free_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr) {
    uint32_t page_index = ((uint32_t)virtual_addr >> 22) & 0x3FF;
    uint32_t physical_addr = (page_dir->table[page_index].lower_address << 22);
    uint32_t frame_index = physical_addr / PAGE_FRAME_SIZE;
    if (page_manager_state.page_frame_map[frame_index]) {
        page_manager_state.page_frame_map[frame_index] = false;
        page_manager_state.free_page_frame_count++;
        page_dir->table[page_index].flag.present_bit = 0;
        flush_single_tlb(virtual_addr);

        return true;
    }
    return false;
}
