#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include"filesystem.h"
#include"file_properties.h"
#define MAX_FILE_LEN 50

void Open_file(char *file) { //for double clicking a file; this function doesn't let the user choose what program to run with
  char dot[10]; //not aware of any extensions longer than 10 chars
  get_file_extension(file, dot);
 printf("%s \n", dot);

  if (strcmp(dot, ".txt") == 0 || strcmp(dot, ".md") == 0 ||
      strcmp(dot, ".c") == 0 || strcmp(dot, ".h") == 0) {

      execlp("gedit", "gedit", file, NULL);

    return;
  }

  if (strcmp(dot, ".png") == 0 || strcmp(dot, ".jpg") == 0) {

      execlp("eog", "eog", file, NULL);

    return;
  }

  else
    printf("Unrecognized file extension. \n");
}
