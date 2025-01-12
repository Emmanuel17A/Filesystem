#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define MAX_INODES 100
#define MAX_BLOCKS 100000
#define BLOCK_SIZE 4096
#define MAX_NAME_LEN 255

// Define the custom inode structure
typedef struct {
    int inode_number;
    size_t size;
    int data_block;
    int is_directory; // 1 if directory, 0 if file
    time_t creation_time;
    char name[MAX_NAME_LEN]; // Store name of the file or directory
} MyInode;

// Define the superblock structure
typedef struct {
    int total_inodes;
    int total_blocks;
    int free_inodes;
    int free_blocks;
} MySuperBlock;

// Define the directory structure
typedef struct DirNode {
    MyInode inode;
    struct DirNode *parent;
    struct DirNode *children[MAX_INODES];
    int num_children;
} DirNode;

// Define the custom file system
typedef struct {
    MyInode inodes[MAX_INODES];
    int num_inodes;
    MySuperBlock superblock; // Superblock
    char disk[MAX_BLOCKS][BLOCK_SIZE]; // disk blocks
    DirNode *root; // Root directory
    DirNode *current_dir; // Current directory
} MyDir;

// Initialize the file system
MyDir* initialize_filesystem(const char *root_path) {
    MyDir *dir = (MyDir *)malloc(sizeof(MyDir));
    if (!dir) return NULL;

    dir->num_inodes = 0;
    dir->superblock.total_inodes = MAX_INODES;
    dir->superblock.total_blocks = MAX_BLOCKS;
    dir->superblock.free_inodes = MAX_INODES;
    dir->superblock.free_blocks = MAX_BLOCKS;

    // Initialize root directory
    dir->root = (DirNode *)malloc(sizeof(DirNode));
    dir->root->inode.inode_number = dir->num_inodes++;
    dir->root->inode.size = 0;
    dir->root->inode.data_block = 0; // Root directory data block
    dir->root->inode.is_directory = 1;
    time(&dir->root->inode.creation_time);
    strncpy(dir->root->inode.name, root_path, MAX_NAME_LEN - 1);
    dir->root->inode.name[MAX_NAME_LEN - 1] = '\0';
    dir->root->parent = NULL;
    dir->root->num_children = 0;

    dir->inodes[dir->root->inode.inode_number] = dir->root->inode;
    dir->current_dir = dir->root;

    return dir;
}

// Add a file or directory to the simulated file system
int add_file(MyDir *dir, const char *name, size_t size, int is_directory) {
    if (dir->superblock.free_inodes <= 0 || dir->superblock.free_blocks <= 0) {
        fprintf(stderr, "No space left to add file\n");
        return -1;
    }

    // Check for duplicate names
    for (int i = 0; i < dir->current_dir->num_children; ++i) {
        if (strcmp(dir->current_dir->children[i]->inode.name, name) == 0) {
            fprintf(stderr, "File name '%s' already exists\n", name);
            return -1;
        }
    }

    int inode_number = dir->num_inodes;
    MyInode new_inode = {inode_number, size, inode_number, is_directory, time(NULL), ""};
    strncpy(new_inode.name, name, MAX_NAME_LEN - 1);
    new_inode.name[MAX_NAME_LEN - 1] = '\0'; // Ensure null-termination

    // Simulate writing file data to disk block
    snprintf(dir->disk[new_inode.data_block], BLOCK_SIZE, "File/Directory: %s", name);

    dir->inodes[inode_number] = new_inode;
    dir->superblock.free_inodes--;
    dir->superblock.free_blocks--;

    DirNode *new_node = (DirNode *)malloc(sizeof(DirNode));
    new_node->inode = new_inode;
    new_node->parent = dir->current_dir;
    new_node->num_children = 0;

    dir->current_dir->children[dir->current_dir->num_children++] = new_node;

    dir->num_inodes++;

    printf("Added '%s' to the file system\n", name);
    return 0;
}

// Remove a file or directory from the simulated file system
int remove_file(MyDir *dir, const char *name) {
    for (int i = 0; i < dir->current_dir->num_children; ++i) {
        if (strcmp(dir->current_dir->children[i]->inode.name, name) == 0) {
            // Free inode and block
            dir->superblock.free_inodes++;
            dir->superblock.free_blocks++;
            // Remove the child from the current directory
            free(dir->current_dir->children[i]);
            for (int j = i; j < dir->current_dir->num_children - 1; ++j) {
                dir->current_dir->children[j] = dir->current_dir->children[j + 1];
            }
            dir->current_dir->num_children--;
            printf("Removed '%s' from the file system\n", name);
            return 0;
        }
    }
    printf("File '%s' not found\n", name);
    return -1;
}

// Rename a file or directory in the simulated file system
int rename_file(MyDir *dir, const char *old_name, const char *new_name) {
    for (int i = 0; i < dir->current_dir->num_children; ++i) {
        if (strcmp(dir->current_dir->children[i]->inode.name, old_name) == 0) {
            strncpy(dir->current_dir->children[i]->inode.name, new_name, MAX_NAME_LEN - 1);
            dir->current_dir->children[i]->inode.name[MAX_NAME_LEN - 1] = '\0'; // Ensure null-termination
            printf("Renamed '%s' to '%s'\n", old_name, new_name);
            return 0;
        }
    }
    printf("File '%s' not found\n", old_name);
    return -1;
}

// Show file properties in a dialog
void show_file_properties(MyDir *dir, const char *name) {
    for (int i = 0; i < dir->current_dir->num_children; ++i) {
        if (strcmp(dir->current_dir->children[i]->inode.name, name) == 0) {
            MyInode inode = dir->current_dir->children[i]->inode;
            char buffer[1024];
            snprintf(buffer, sizeof(buffer),
                     "Name: %s\nSize: %zu bytes\nCreation Time: %s",
                     inode.name, inode.size, ctime(&inode.creation_time));
            GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
                                                       GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                                       "%s", buffer);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return;
        }
    }
    printf("File '%s' not found\n", name);
}

// Load content from the real file system
void load_real_directory(MyDir *dir, const char *path) {
    DIR *dp;
    struct dirent *entry;
    struct stat file_info;

    if ((dp = opendir(path)) == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dp)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (stat(full_path, &file_info) == -1) {
            perror("stat");
            continue;
        }

        add_file(dir, entry->d_name, file_info.st_size, S_ISDIR(file_info.st_mode));
    }

    closedir(dp);
}

// Change directory
int change_directory(MyDir *dir, const char *name) {
    if (strcmp(name, "..") == 0) {
        if (dir->current_dir->parent != NULL) {
            dir->current_dir = dir->current_dir->parent;
            printf("Changed to directory '%s'\n", dir->current_dir->inode.name);
            return 0;
        } else {
            printf("Already at the root directory\n");
            return -1;
        }
    }

    for (int i = 0; i < dir->current_dir->num_children; i++) {
        if (dir->current_dir->children[i]->inode.is_directory && strcmp(dir->current_dir->children[i]->inode.name, name) == 0) {
            dir->current_dir = dir->current_dir->children[i];
            printf("Changed to directory '%s'\n", dir->current_dir->inode.name);
            return 0;
        }
    }

    printf("Directory '%s' not found\n", name);
    return -1;
}

DirNode* create_dir_node(MyDir *fs, const char *name) {
    // Find a free inode
    int inode_index = -1;
    for (int i = 0; i < MAX_INODES; i++) {
        if (fs->inodes[i].inode_number == -1) { // Assuming -1 means free
            inode_index = i;
            break;
        }
    }

    if (inode_index == -1) {
        printf("No free inodes available\n");
        return NULL;
    }

    // Initialize the new inode
    MyInode *new_inode = &fs->inodes[inode_index];
    new_inode->inode_number = inode_index;
    strncpy(new_inode->name, name, MAX_NAME_LEN - 1);
    new_inode->name[MAX_NAME_LEN - 1] = '\0';
    new_inode->size = 0;
    new_inode->data_block = -1;
    new_inode->is_directory = 1;
    new_inode->creation_time = time(NULL);

    // Create the new directory node
    DirNode *new_node = (DirNode *)malloc(sizeof(DirNode));
    new_node->inode = *new_inode;
    new_node->parent = NULL;
    new_node->num_children = 0;

    fs->superblock.free_inodes--;
    return new_node;
}

DirNode* find_dir_node_by_name(DirNode *parent, const char *name) {
    for (int i = 0; i < parent->num_children; i++) {
        if (strcmp(parent->children[i]->inode.name, name) == 0) {
            return parent->children[i];
        }
    }
    return NULL;
}

void add_dir_node_to_parent(DirNode *parent, DirNode *child) {
    if (parent->num_children < MAX_INODES) {
        parent->children[parent->num_children++] = child;
        child->parent = parent;
    } else {
        printf("Max children reached for parent directory\n");
    }
}

int my_mkdir(MyDir *fs, const char *name) {
    if (fs->current_dir->num_children >= MAX_INODES) {
        printf("Maximum number of children exceeded in the current directory\n");
        return -1;
    }

    // Check for duplicate directory name
    for (int i = 0; i < fs->current_dir->num_children; i++) {
        if (strcmp(fs->current_dir->children[i]->inode.name, name) == 0) {
            printf("Directory with the name '%s' already exists\n", name);
            return -1;
        }
    }

    DirNode *new_node = create_dir_node(fs, name);
    if (!new_node) {
        return -1;
    }

    add_dir_node_to_parent(fs->current_dir, new_node);
    return 0;
}

int my_rmdir(MyDir *fs, const char *name) {
    DirNode *target = find_dir_node_by_name(fs->current_dir, name);
    if (!target) {
        printf("Directory not found: %s\n", name);
        return -1;
    }

    if (target->num_children > 0) {
        printf("Directory is not empty: %s\n", name);
        return -1;
    }

    // Remove the directory
    int inode_index = target->inode.inode_number;
    fs->inodes[inode_index].inode_number = -1;
    fs->superblock.free_inodes++;

    // Remove the directory node from the parent's children
    for (int i = 0; i < fs->current_dir->num_children; i++) {
        if (fs->current_dir->children[i] == target) {
            for (int j = i; j < fs->current_dir->num_children - 1; j++) {
                fs->current_dir->children[j] = fs->current_dir->children[j + 1];
            }
            fs->current_dir->num_children--;
            break;
        }
    }

    free(target);
    return 0;
}

// Function declarations for callback handlers
void createfile_callback(GtkWidget *widget, gpointer data);
void createfolder_callback(GtkWidget *widget, gpointer data);
void executefile_callback(GtkWidget *widget, gpointer data);
void deletefile_callback(GtkWidget *widget, gpointer data);
void renamefile_callback(GtkWidget *widget, gpointer data);
void view_props_callback(GtkWidget *widget, gpointer data);

// Define the file system
MyDir *fs;

// GUI components
GtkWidget *window;
GtkWidget *treeview;
GtkTreeStore *treestore;

// Function to update the treeview with the current directory contents
void update_treeview(MyDir *dir) {
    gtk_tree_store_clear(treestore);

    GtkTreeIter iter;
    for (int i = 0; i < dir->current_dir->num_children; ++i) {
        gtk_tree_store_append(treestore, &iter, NULL);
        gtk_tree_store_set(treestore, &iter, 0, dir->current_dir->children[i]->inode.name, -1);
    }
}

// Callback for creating a new file
void createfile_callback(GtkWidget *widget, gpointer data) {
    add_file(fs, "NewFile.txt", 100, 0);
    update_treeview(fs);
}

// Callback for creating a new folder
void createfolder_callback(GtkWidget *widget, gpointer data) {
    my_mkdir(fs, "NewFolder");
    update_treeview(fs);
}

// Callback for executing a file
void executefile_callback(GtkWidget *widget, gpointer data) {
    // Implementation for executing a file
    printf("Execute file\n");
}

// Callback for deleting a file or folder
void deletefile_callback(GtkWidget *widget, gpointer data) {
    remove_file(fs, "NewFile.txt");
    update_treeview(fs);
}

// Callback for renaming a file or folder
void renamefile_callback(GtkWidget *widget, gpointer data) {
    rename_file(fs, "NewFile.txt", "RenamedFile.txt");
    update_treeview(fs);
}

// Callback for viewing file properties
void view_props_callback(GtkWidget *widget, gpointer data) {
    show_file_properties(fs, "NewFile.txt");
}

// Create the GUI window
void create_window(MyDir *dir) {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Virtual File System GUI");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    treestore = gtk_tree_store_new(1, G_TYPE_STRING);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(treestore));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    update_treeview(dir);

    GtkWidget *treeview_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(treeview_scrolled), treeview);
    gtk_box_pack_start(GTK_BOX(vbox), treeview_scrolled, TRUE, TRUE, 0);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);

    GtkWidget *button_createfile = gtk_button_new_with_label("Create File");
    g_signal_connect(button_createfile, "clicked", G_CALLBACK(createfile_callback), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button_createfile, TRUE, TRUE, 0);

    GtkWidget *button_createfolder = gtk_button_new_with_label("Create Folder");
    g_signal_connect(button_createfolder, "clicked", G_CALLBACK(createfolder_callback), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button_createfolder, TRUE, TRUE, 0);

    GtkWidget *button_deletefile = gtk_button_new_with_label("Delete File");
    g_signal_connect(button_deletefile, "clicked", G_CALLBACK(deletefile_callback), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button_deletefile, TRUE, TRUE, 0);

    GtkWidget *button_renamefile = gtk_button_new_with_label("Rename File");
    g_signal_connect(button_renamefile, "clicked", G_CALLBACK(renamefile_callback), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button_renamefile, TRUE, TRUE, 0);

    GtkWidget *button_viewprops = gtk_button_new_with_label("View Properties");
    g_signal_connect(button_viewprops, "clicked", G_CALLBACK(view_props_callback), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button_viewprops, TRUE, TRUE, 0);

    gtk_widget_show_all(window);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Initialize the file system
    fs = initialize_filesystem("/");

    // Load real directory content (optional)
    load_real_directory(fs, ".");

    // Create the GUI window
    create_window(fs);

    gtk_main();
    return 0;
}
