#ifndef FILE_PROPERTIES_H_INCLUDED
#define FILE_PROPERTIES_H_INCLUDED
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
//#include"Custom_functions.h"
#define MAX_FILE_LEN 50


struct file_properties {
  char perms[9];
  int size_bytes;
  char size[10];
  int isdir;
};

int new_file(const char *name);
int new_folder(const char *name);
void remove_contents(const char *Content_name);
char ** get_files(int *num_files);
int check_if_file_exist(const char *filename);
void get_file_extension(const char *filename, char *buff);
void get_file_properties(const char *filename, struct file_properties * props);

#endif // FILE_PROPERTIES_H_INCLUDED
