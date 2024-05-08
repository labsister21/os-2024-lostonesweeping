#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../header/text/framebuffer.h"
#include "../../header/stdlib/string.h"
#include "../../header/cpu/portio.h"

struct FramebufferState framebuffer_state = {
    .row = 0, 
    .col = 0, 
    .fg = 0xF, 
    .bg = 0
};

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    uint16_t pos = r * BUFFER_WIDTH + c; 
    out(CURSOR_PORT_CMD, 0x0F);
	out(CURSOR_PORT_DATA, (uint8_t) (pos & 0xFF));
	out(CURSOR_PORT_CMD, 0x0E);
	out(CURSOR_PORT_DATA, (uint8_t) ((pos >> 8) & 0xFF));

    framebuffer_state.row = r; 
    framebuffer_state.col = c;
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    framebuffer_set_cursor(row, col);
    uint16_t idx = (row * BUFFER_WIDTH) +  col; 
    uint16_t *where = (uint16_t *)BASE_MEMORY_OFFSET + idx;
    uint16_t attrib = (bg << 4) | (fg & 0x0F);
    *where = c | (attrib << 8);
}


void framebuffer_clear_delete(void){
    if (framebuffer_state.col > 0) {
        framebuffer_state.col--;  
        framebuffer_write(
            framebuffer_state.row, 
            framebuffer_state.col, 
            '\0',  
            0xF,   
            0x0    
        );
    } 
    else if (framebuffer_state.row > 0) {
        framebuffer_state.row--;  
        size_t prev_col = BUFFER_WIDTH - 1; 
        uint16_t *prev_row_start = (uint16_t *)(BASE_MEMORY_OFFSET + (framebuffer_state.row * BUFFER_WIDTH * 2));
        while(prev_col > 0 && prev_row_start[prev_col] == 0x00){
            prev_col--;
        }
        framebuffer_state.col = prev_col;
        framebuffer_write(
            framebuffer_state.row, 
            framebuffer_state.col, 
            '\0',  
            0xF,   
            0x0    
        );
    }
  
}

void framebuffer_newline(void){
    framebuffer_state.row++;
    if(framebuffer_state.row == BUFFER_HEIGHT){
        framebuffer_state.row = 0;
    }
    framebuffer_set_cursor(
        framebuffer_state.row, 
        0
    );
}

void framebuffer_move_cursor(enum FramebufferCursorMove direction, int count) {
	int next_row = framebuffer_state.row;
	int next_col = framebuffer_state.col;
	switch (direction) {
	case UP: {
		next_row -= count;
	} break;
	case DOWN: {
		next_row += count;
	} break;
	case LEFT: {
		next_col -= count;
	} break;
	case RIGHT: {
		next_col += count;
	} break;
	}
	framebuffer_set_cursor(next_row, next_col);
};

void framebuffer_put(char c){
    framebuffer_write(
        framebuffer_state.row, 
        framebuffer_state.col, 
        c, 
        framebuffer_state.fg, 
        framebuffer_state.bg
    );

    framebuffer_set_cursor(framebuffer_state.row, framebuffer_state.col);
    framebuffer_state.col += 1;
    if (framebuffer_state.col == BUFFER_WIDTH) {
        framebuffer_state.col = 0;
        framebuffer_state.row += 1;
    }
    if (framebuffer_state.row == BUFFER_HEIGHT) {
        framebuffer_state.row = 0;
    }
}

void framebuffer_clear(void) {
    size_t framebuffer_size = BUFFER_WIDTH * BUFFER_HEIGHT + sizeof(FRAMEBUFFER_MEMORY_OFFSET);
    memset(FRAMEBUFFER_MEMORY_OFFSET, 0, framebuffer_size);
}


