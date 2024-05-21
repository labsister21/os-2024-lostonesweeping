#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define FRAMEBUFFER_MEMORY_OFFSET ((uint8_t*) 0xC00B8000)
#define BASE_MEMORY_OFFSET 0xB8000
#define BUFFER_WIDTH 80
#define BUFFER_HEIGHT 100
#define CURSOR_PORT_CMD 0x03D4
#define CURSOR_PORT_DATA 0x03D5

/**
 * Terminal framebuffer
 * Resolution: 80x25
 * Starting at FRAMEBUFFER_MEMORY_OFFSET,
 * - Even number memory: Character, 8-bit
 * - Odd number memory:  Character color lower 4-bit, Background color upper 4-bit
*/

/**
 * Set framebuffer character and color with corresponding parameter values.
 * More details: https://en.wikipedia.org/wiki/BIOS_color_attributes
 *
 * @param row Vertical location (index start 0)
 * @param col Horizontal location (index start 0)
 * @param c   Character
 * @param fg  Foreground / Character color
 * @param bg  Background color
 */

struct FramebufferState{
    int row; 
    int col; 
    uint8_t fg; 
    uint8_t bg; 

}__attribute((packed)); 

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg);

/**
 * Set cursor to specified location. Row and column starts from 0
 * 
 * @param r row
 * @param c column
*/
void framebuffer_set_cursor(uint8_t r, uint8_t c);

/**
 * Set all cell in framebuffer character to 0x00 (empty character)
 * and color to 0x07 (gray character & black background)
 * Extra note: It's allowed to use different color palette for this
 *
 */

void framebuffer_scroll_up(void);

void framebuffer_scroll_down(void);

void framebuffer_clear(void);

void framebuffer_put(char c, uint8_t color);

void framebuffer_clear_delete(void); 

void framebuffer_newline(void);
    
#endif