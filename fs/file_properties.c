#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include"file_properties.h"


int new_file(const char *name) {
  int fd = open(name, O_CREAT | O_EXCL, 0666);
  printf("fd: %d \n", fd);

  if (fd < 0)
    return 0;

  printf("File created! \n");
  return 1;
}

int new_folder(const char *name) {
  int folder = mkdir(name);

  if (folder < 0)
    return 0;

  printf("Folder created! \n");
  return 1;
}

void remove_contents(const char *content_name) {
  int status = remove( content_name);
  if (status < 0)
    printf("Error %d: %s", errno, strerror(errno));
}


char ** get_files(int *num_files) {
  char ** files = malloc(100 * sizeof(char *)); //can be any size but I doubt we'll be testing in a folder w/ > 100 files/folders
    DIR* dir = opendir("."); //current working directory for now as a proof of concept
  struct dirent * info = readdir(dir);
  while (info != NULL) {
    if (info -> d_name[0] != '.') { //conditional gets rid of hidden files/folders
      files[*num_files] = (char *)malloc(MAX_FILE_LEN);
      strncpy(files[*num_files], info -> d_name, MAX_FILE_LEN);
      (*num_files)++;
    }

    info = readdir(dir); //move on to the next entry
  }

  files[*num_files] = NULL; //end the array in case there is extra stuff stored from earlier runs of this function
  closedir(dir);
  return files;
}

int check_if_file_exist(const char *filename) {
  DIR *dir = opendir(".");
  struct dirent *info = readdir(dir);
  while (info != NULL) {
    if (strcmp(filename, info -> d_name) == 0)
      return 1;

    info = readdir(dir);
  }

  return 0;
}

void get_file_extension(const char *filename, char *buff) {
  int i = strlen(filename) - 1;
  while (i >= 0) { //loop backwards to find the '.'; not aware of a backwards strsep and this is easy anyway
    if (filename[i] == '.') {
      strncpy(buff, filename + i, 10); //filename + i makes it so that strncpy only starts copying from the period onwards
      return; //exit the loop since the extension has been found
    }

    i--;
  }

	strcpy(buff, "\0"); //period not found; make the extension a null-ended string which gets returned
}

void get_file_properties(const char *filename, struct file_properties * props) {
  struct stat metadata;
  stat(filename, &metadata);

  //edit every individual byte of props -> perms string in the file properties struct
  props -> perms[0] = (metadata.st_mode & S_IRUSR) ? 'r' : '-';
  props -> perms[1] = (metadata.st_mode & S_IWUSR) ? 'w' : '-';
  props -> perms[2] = (metadata.st_mode & S_IXUSR) ? 'x' : '-';
  props -> perms[3] = (metadata.st_mode & S_IRGRP) ? 'r' : '-';
  props -> perms[4] = (metadata.st_mode & S_IWGRP) ? 'w' : '-';
  props -> perms[5] = (metadata.st_mode & S_IXGRP) ? 'x' : '-';
  props -> perms[6] = (metadata.st_mode & S_IROTH) ? 'r' : '-';
  props -> perms[7] = (metadata.st_mode & S_IWOTH) ? 'w' : '-';
  props -> perms[8] = (metadata.st_mode & S_IXOTH) ? 'x' : '-';
  props -> perms[9] = '\0'; //for some reason there's extra bytes to the right of the S_IXOTH byte even though the string is 9 large, whatever this fixes it

  props -> size_bytes = metadata.st_size;

  if (props -> size_bytes > 1000) {
    char buf[10];
    if (props -> size_bytes > 1000000000) { //1 GB
      float size = props -> size_bytes / 1000000000.0;
      sprintf(buf, "%.1f", size);
      strncat(buf, " GB", 4);
    } else if (props -> size_bytes > 1000000) { // 1MB
      float size = props -> size_bytes / 1000000.0;
      sprintf(buf, "%.1f", size);
      strncat(buf, " MB", 4);
    }

    float size = props -> size_bytes / 1000.0;
    sprintf(buf, "%.1f", size);
    strncat(buf, " KB", 4);

    strncpy(props -> size, buf, 10);
  }

  props -> isdir = (S_ISDIR(metadata.st_mode)) ? 1 : 0;
}
