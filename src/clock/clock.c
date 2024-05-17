#include "header/driver/cmos.h"
#include <stdint.h>

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

void delay(unsigned int milliseconds) {
    for (unsigned int i = 0; i < milliseconds; ++i) {
        // Approximate delay by executing empty loop
        for (volatile int j = 0; j < 10000; ++j) {
            // Do nothing
        }
    }
}

void print_time(int hour, int minute, int second) {
    // Extract individual digits of hour
    int hour_tens = (hour + 7) / 10;
    int hour_ones = (hour + 7) % 10;
    // Convert digits to characters
    char hour_tens_char = hour_tens + '0';
    char hour_ones_char = hour_ones + '0';

    // Extract individual digits of minute
    int minute_tens = minute / 10;
    int minute_ones = minute % 10;
    // Convert digits to characters
    char minute_tens_char = minute_tens + '0';
    char minute_ones_char = minute_ones + '0';

    // Extract individual digits of second
    int second_tens = second / 10;
    int second_ones = second % 10;
    // Convert digits to characters
    char second_tens_char = second_tens + '0';
    char second_ones_char = second_ones + '0';

    // Print the time characters
    syscall(8, (uint32_t)hour_tens_char, (uint32_t)68, 0);
    syscall(8, (uint32_t)hour_ones_char, (uint32_t)69, 0);
    syscall(8, (uint32_t)':', (uint32_t)70, 0);
    syscall(8, (uint32_t)minute_tens_char, (uint32_t)71, 0);
    syscall(8, (uint32_t)minute_ones_char, (uint32_t)72, 0);
    syscall(8, (uint32_t)':', (uint32_t)73, 0);
    syscall(8, (uint32_t)second_tens_char, (uint32_t)74, 0);
    syscall(8, (uint32_t)second_ones_char, (uint32_t)75, 0);
}
    
int main(void) {
    uint8_t hour, minute, second;

while (1) {
        // syscall(8, (uint32_t)'L', (uint32_t)47, 0);
        read_rtc(&hour, &minute, &second); // Read time from RTC
        
        // Adjust the hour according to 12-hour format if needed
        int adjusted_hour = hour % 12;
        if (adjusted_hour == 0) {
            adjusted_hour = 12;
        }

        print_time(adjusted_hour, minute, second);

        // Delay for approximately 1 second
        delay(1000);
    }
    return 0;
}