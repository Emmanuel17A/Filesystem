#ifndef FILESYSTEMUL_H_INCLUDED
#define FILESYSTEMUL_H_INCLUDED

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
    char disk[MAX_BLOCKS][BLOCK_SIZE];// disk blocks
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
        printf("No free inodes available.\n");
        return NULL;
    }

    // Initialize inode
    MyInode *inode = &fs->inodes[inode_index];
    inode->inode_number = inode_index;
    inode->size = 0;
    inode->data_block = -1; // Not used for directories
    inode->is_directory = 1;
    inode->creation_time = time(NULL);
    strncpy(inode->name, name, MAX_NAME_LEN);

    // Initialize directory node
    DirNode *dir_node = malloc(sizeof(DirNode));
    if (!dir_node) {
        printf("Memory allocation failed.\n");
        return NULL;
    }

    dir_node->inode = *inode;
    dir_node->parent = fs->current_dir;
    dir_node->num_children = 0;

    return dir_node;
}



// Function to create a directory
void create_directory(MyDir *fs, const char *name) {
    if (!fs->current_dir) {
        printf("No current directory set.\n");
        return;
    }

    // Check if the directory already exists
    for (int i = 0; i < fs->current_dir->num_children; i++) {
        if (strcmp(fs->current_dir->children[i]->inode.name, name) == 0) {
            printf("Directory already exists.\n");
            return;
        }
    }

    // Create the new directory
    DirNode *new_dir = create_dir_node(fs, name);
    if (!new_dir) {
        return;
    }

    // Add the new directory to the current directory's children
    if (fs->current_dir->num_children < MAX_INODES) {
        fs->current_dir->children[fs->current_dir->num_children++] = new_dir;
    } else {
        printf("Too many directories in current directory.\n");
        free(new_dir);
    }
}

// Update the GUI list store with the current directory contents
void update_directory_list(MyDir *dir, GtkListStore *list_store) {
    gtk_list_store_clear(list_store);
    for (int i = 0; i < dir->current_dir->num_children; ++i) {
        GtkTreeIter iter;
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter,
                           0, dir->current_dir->children[i]->inode.name,
                           1, dir->current_dir->children[i]->inode.is_directory ? "Directory" : "File",
                           -1);
    }
}

// Button callback to go up to the parent directory
void on_go_up_button_clicked(GtkWidget *button, gpointer data) {
    MyDir *dir = (MyDir *)g_object_get_data(G_OBJECT(button), "filesystem");
    GtkListStore *list_store = (GtkListStore *)g_object_get_data(G_OBJECT(button), "list_store");

    if (change_directory(dir, "..") == 0) {
        update_directory_list(dir, list_store);
    }
}

// Callback for the "Enter Directory" button
void on_enter_directory_button_clicked(GtkButton *button, GtkEntry *entry) {
    const char *dir_name = gtk_entry_get_text(entry);
    MyDir *dir = g_object_get_data(G_OBJECT(button), "filesystem");
    if (change_directory(dir, dir_name) == 0) {
        GtkListStore *list_store = GTK_LIST_STORE(g_object_get_data(G_OBJECT(button), "list_store"));
        update_directory_list(dir, list_store);
    }
}

// Button callback to show file properties
void on_show_properties_button_clicked(GtkWidget *button, gpointer data) {
    MyDir *dir = (MyDir *)g_object_get_data(G_OBJECT(button), "filesystem");
    const char *filename = gtk_entry_get_text(GTK_ENTRY(data));
    show_file_properties(dir, filename);
}

// Button callback to rename a file
void on_rename_file_button_clicked(GtkWidget *button, gpointer data) {
    MyDir *dir = (MyDir *)g_object_get_data(G_OBJECT(button), "filesystem");
    GtkEntry **entries = (GtkEntry **)data;
    const char *old_name = gtk_entry_get_text(entries[0]);
    const char *new_name = gtk_entry_get_text(entries[1]);
    rename_file(dir, old_name, new_name);

    GtkListStore *list_store = (GtkListStore *)g_object_get_data(G_OBJECT(button), "list_store");
    update_directory_list(dir, list_store);
}

// Button callback to create a new file
void on_create_file_button_clicked(GtkWidget *button, gpointer data) {
    MyDir *dir = (MyDir *)g_object_get_data(G_OBJECT(button), "filesystem");
    const char *filename = gtk_entry_get_text(GTK_ENTRY(data));
    add_file(dir, filename, 0, 0); // Create an empty file

    GtkListStore *list_store = (GtkListStore *)g_object_get_data(G_OBJECT(button), "list_store");
    update_directory_list(dir, list_store);
}

// Button callback to delete a file
void on_delete_file_button_clicked(GtkWidget *button, gpointer data) {
    MyDir *dir = (MyDir *)g_object_get_data(G_OBJECT(button), "filesystem");
    const char *filename = gtk_entry_get_text(GTK_ENTRY(data));
    remove_file(dir, filename);

    GtkListStore *list_store = (GtkListStore *)g_object_get_data(G_OBJECT(button), "list_store");
    update_directory_list(dir, list_store);
}
void on_create_directory_button_clicked(GtkButton *button, gpointer user_data) {
    GtkEntry *entry = GTK_ENTRY(user_data);
    const gchar *dirname = gtk_entry_get_text(entry);
    MyDir *fs = g_object_get_data(G_OBJECT(button), "filesystem");
    GtkListStore *list_store = g_object_get_data(G_OBJECT(button), "list_store");

    if (strlen(dirname) > 0) {
        create_directory(fs, dirname);
        update_directory_list(fs, list_store); // Update the tree view to reflect the new directory
    } else {
        printf("Directory name cannot be empty.\n");
    }
}
void initialize_gui(MyDir *dir) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *button;
    GtkWidget *entry;
    GtkWidget *scrolled_window;
    GtkWidget *tree_view;
    GtkWidget *label;
    GtkWidget *property_button;
    GtkWidget *rename_hbox;
    GtkWidget *create_entry;
    GtkWidget *delete_entry;

    gtk_init(NULL, NULL);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "File System Simulator");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Navigation buttons and entry
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    button = gtk_button_new_with_label("Go Up");
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);
    g_signal_connect(button, "clicked", G_CALLBACK(on_go_up_button_clicked), NULL);
    g_object_set_data(G_OBJECT(button), "filesystem", dir);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Enter Directory");
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);
    g_signal_connect(button, "clicked", G_CALLBACK(on_enter_directory_button_clicked), entry);
    g_object_set_data(G_OBJECT(button), "filesystem", dir);

    // File properties button
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    label = gtk_label_new("File:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 5);

    property_button = gtk_button_new_with_label("Show Properties");
    gtk_box_pack_start(GTK_BOX(hbox), property_button, FALSE, FALSE, 5);
    g_signal_connect(property_button, "clicked", G_CALLBACK(on_show_properties_button_clicked), entry);
    g_object_set_data(G_OBJECT(property_button), "filesystem", dir);

    // Rename file
    rename_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), rename_hbox, FALSE, FALSE, 5);

    label = gtk_label_new("Rename File:");
    gtk_box_pack_start(GTK_BOX(rename_hbox), label, FALSE, FALSE, 5);

    GtkWidget *old_name_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(rename_hbox), old_name_entry, TRUE, TRUE, 5);

    GtkWidget *new_name_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(rename_hbox), new_name_entry, TRUE, TRUE, 5);

    GtkWidget *rename_button = gtk_button_new_with_label("Rename");
    gtk_box_pack_start(GTK_BOX(rename_hbox), rename_button, FALSE, FALSE, 5);

    GtkEntry *entries[2] = {GTK_ENTRY(old_name_entry), GTK_ENTRY(new_name_entry)};
    g_signal_connect(rename_button, "clicked", G_CALLBACK(on_rename_file_button_clicked), entries);
    g_object_set_data(G_OBJECT(rename_button), "filesystem", dir);

    // Create file
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    label = gtk_label_new("Create File:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    create_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), create_entry, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Create");
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);
    g_signal_connect(button, "clicked", G_CALLBACK(on_create_file_button_clicked), create_entry);
    g_object_set_data(G_OBJECT(button), "filesystem", dir);

    // Delete file
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    label = gtk_label_new("Delete File:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    delete_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), delete_entry, TRUE, TRUE, 5);

    button = gtk_button_new_with_label("Delete");
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 5);
    g_signal_connect(button, "clicked", G_CALLBACK(on_delete_file_button_clicked), delete_entry);
    g_object_set_data(G_OBJECT(button), "filesystem", dir);

    // Create and set up the list store
    GtkListStore *list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 5);

    tree_view = gtk_tree_view_new();
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), GTK_TREE_MODEL(list_store));

    // Store list_store in button data for callback functions
    g_object_set_data(G_OBJECT(button), "list_store", list_store);
    g_object_set_data(G_OBJECT(rename_button), "list_store", list_store);
    g_object_set_data(G_OBJECT(property_button), "list_store", list_store);

    update_directory_list(dir, list_store);

    gtk_widget_show_all(window);
    gtk_main();
}






int main(int argc, char *argv[]) {
    const char *root_path = ".";
    if (argc > 1) {
        root_path = argv[1];
    }

    MyDir *filesystem = initialize_filesystem(root_path);
    if (!filesystem) {
        fprintf(stderr, "Failed to initialize file system\n");
        return EXIT_FAILURE;
    }

    load_real_directory(filesystem, root_path);
    initialize_gui(filesystem);

    return EXIT_SUCCESS;
}


#endif // FILESYSTEMUL_H_INCLUDED
