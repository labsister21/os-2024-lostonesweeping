#ifndef UTIL_H
#define UTIL_H
#include "user-shell.h"


void clearBuf(void *pointer, size_t n);

void print_curr_dir(char* path_str, uint32_t current_dir);

void put_char(char buf);

void put_chars(char* buf);

int findEntryName(char* name);

bool isPathAbsolute(char* prompt_val, int (*prompt_info)[2], int prompt_pos); 

void extractDirectories(char* dir, char directories[][8], int* numDirs);

void extract_dir_special(char* dir, char directories[][12], int* numDirs);

void extractBaseName(const char *filename, char *basename);

void extractExtension(const char *filename, char *extension);


#endif