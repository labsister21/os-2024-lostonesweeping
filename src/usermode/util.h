#ifndef UTIL_H
#define UTIL_H
#include "user-shell.h"


void clearBuf(void *pointer, size_t n);

void print_curr_dir(char* path_str, uint32_t current_dir);

void put_char(char buf);

void put_chars(char* buf, uint8_t color);

int findEntryName(char* name);


void extract_dir(char* dir, char directories[][12], int* numDirs);

void extract_filename(const char *filename, char *basename);

void extract_file_extension(const char *filename, char *extension);

// void extractDirectories(char* dir, char directories[][8], int* numDirs);
#endif