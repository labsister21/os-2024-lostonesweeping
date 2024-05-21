#include "header/driver/cmos.h"

void out_byte(int port, int value) {
    asm volatile ("outb %b0, %w1" : : "a" ((char)value), "d" ((unsigned short)port));
}

int in_byte(int port) {
    int result;
    asm volatile ("inb %w1, %b0" : "=a" (result) : "d" ((unsigned short)port));
    return result;
}

enum {
      cmos_address = 0x70,
      cmos_data    = 0x71
};
 
int get_update_in_progress_flag() {
      out_byte(cmos_address, 0x0A);
      return (in_byte(cmos_data) & 0x80);
}
 
unsigned char get_RTC_register(int reg) {
      out_byte(cmos_address, reg);
      return in_byte(cmos_data);
}

void read_rtc(unsigned char *hour_ptr, unsigned char *minute_ptr, unsigned char *second_ptr) {
    unsigned char last_second, last_minute, last_hour, registerB;

    // Wait until there's no update in progress
    while (get_update_in_progress_flag());

    // Read hours, minutes, and seconds
    *second_ptr = get_RTC_register(0x00);
    *minute_ptr = get_RTC_register(0x02);
    *hour_ptr = get_RTC_register(0x04);

    // Check for updates until there's no change in values
    do {
        // Store previous values
        last_second = *second_ptr;
        last_minute = *minute_ptr;
        last_hour = *hour_ptr;

        // Wait until there's no update in progress
        while (get_update_in_progress_flag());

        // Read hours, minutes, and seconds again
        *second_ptr = get_RTC_register(0x00);
        *minute_ptr = get_RTC_register(0x02);
        *hour_ptr = get_RTC_register(0x04);
    } while ((last_second != *second_ptr) || (last_minute != *minute_ptr) || (last_hour != *hour_ptr));

    // Read Register B
    registerB = get_RTC_register(0x0B);

    // Convert BCD to binary values if necessary
    if (!(registerB & 0x04)) {
        *second_ptr = (*second_ptr & 0x0F) + ((*second_ptr / 16) * 10);
        *minute_ptr = (*minute_ptr & 0x0F) + ((*minute_ptr / 16) * 10);
        *hour_ptr = ((*hour_ptr & 0x0F) + (((*hour_ptr & 0x70) / 16) * 10)) | (*hour_ptr & 0x80);
    }

    // Convert 12 hour clock to 24 hour clock if necessary
    if (!(registerB & 0x02) && (*hour_ptr & 0x80)) {
        *hour_ptr = ((*hour_ptr & 0x7F) + 12) % 24;
    }
}
