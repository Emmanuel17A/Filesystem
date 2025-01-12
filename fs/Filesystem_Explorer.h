
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include""

// Unix-like systems implementation
// Define the custom dirent structure
typedef struct {
    char d_name[256]; // Name of the directory entry
} MyDirent;

// Define the inode structure
typedef struct {
    int inode_number;
    size_t size;
    int data_block;
} MyInode;

// Define the superblock structure
typedef struct {
    int total_inodes;
    int total_blocks;
    int free_inodes;
    int free_blocks;
} MySuperBlock;

// Define the cross-platform directory interface
typedef struct {
    int fd; // File descriptor for Unix-like systems
    void *handle; // Handle for Windows
    char buffer[4096]; // Buffer for reading entries
    size_t buffer_size; // Size of the buffer
    size_t buffer_pos; // Current position in the buffer
    char directory_path[256]; // Path of the directory
    int num_entries; // Number of entries in the directory
    MyInode *inodes; // Array of inodes
    int num_inodes; // Number of inodes
    MySuperBlock superblock; // Superblock
} MyDir;


MyDir* my_opendir(const char *name);
int my_closedir(MyDir *dir);
MyDirent* my_readdir(MyDir *dir);



MyDir* my_opendir(const char *name) {
    MyDir *dir = (MyDir *)malloc(sizeof(MyDir));
    if (!dir) return NULL;
    dir->fd = open(name, O_RDONLY);
    if (dir->fd == -1) {
        free(dir);
        return NULL;
    }
    dir->buffer_size = 0;
    dir->buffer_pos = 0;
    strncpy(dir->directory_path, name, sizeof(dir->directory_path));
    dir->num_entries = 0;
    dir->num_inodes = 0;
    dir->inodes = NULL;
    dir->superblock.total_inodes = 1000;
    dir->superblock.total_blocks = 1000;
    dir->superblock.free_inodes = 10;
    dir->superblock.free_blocks = 10;
    return dir;
}

int my_closedir(MyDir *dir) {
    if (close(dir->fd) == -1) {
        return -1;
    }
    free(dir->inodes); // Free the inodes array
    free(dir);
    return 0;
}

MyDirent* my_readdir(MyDir *dir) {
    static MyDirent entry;
    if (dir->buffer_pos >= dir->buffer_size) {
        dir->buffer_size = my_filesysetm_syscall(mySYS_getdents, dir->fd, dir->buffer, sizeof(dir->buffer));
        if (dir->buffer_size <= 0) {
            return NULL;
        }
        dir->buffer_pos = 0;
    }

    struct my_dirent *d = (struct my_dirent *)(dir->buffer + dir->buffer_pos);
    dir->buffer_pos += 10;//d->d_reclen;
    strncpy(entry.d_name, d->d_name, sizeof(entry.d_name));
    dir->num_entries++;

    // Update inodes array
    dir->num_inodes++;
    dir->inodes = (MyInode *)realloc(dir->inodes, dir->num_inodes * sizeof(MyInode));
    dir->inodes[dir->num_inodes - 1].inode_number = dir->num_inodes;
    dir->inodes[dir->num_inodes - 1].size = 0; // Placeholder
    dir->inodes[dir->num_inodes - 1].data_block = dir->num_inodes;

    // Update superblock
    dir->superblock.free_inodes--;
    dir->superblock.free_blocks--;

    return &entry;
}

#endif
