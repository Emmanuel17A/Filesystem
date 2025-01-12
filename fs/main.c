#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include "filesystem.h"
//#include "file_properties.h"
#define file_icon_image "D:\\Filesystem\\system_images\\fileicon.png"
#define folder_icon_image "D:\\Filesystem\\system_images\\foldericon.png"
#define MAX_FILE_LEN 50

GtkApplication *app;
GtkWidget *windoww; //so you can close window outside activate function
int is_cd = 0;

struct data {
  char filename[50];
  int isDir, col, row;
  GtkWidget *grid;
  GtkWidget *btn;
  GtkWidget *label;
  GtkWidget *popover;
  int called_from_popover;
  GtkWidget *box2;
  GtkWidget *window;
  struct file_properties metadata;
};

int num_files;
char ** files;
struct data icon_location;

gboolean btn_press(GtkWidget *btn, GdkEventButton *event, gpointer userdata);

void free_files() {
  int i;
  for (i = 0; i < num_files; i++)
    free(files[i]);

  free(files);
}

void executefile(GtkWidget *menuitem, gpointer userdata) {
  struct data *d = (struct data *)userdata;
  //char *filename = d -> filename;

  if (d -> isDir == 0)
    Open_file( d -> filename);
  else {
    is_cd = 1;
    chdir(d -> filename);
    gtk_window_close(GTK_WINDOW(windoww));
    g_application_quit(G_APPLICATION(app));
  }
}

void deletefile(GtkWidget *menuitem, gpointer userdata) {
  struct data *d = (struct data *)userdata;
  char *filename = d -> filename;
  remove_contents(filename);
  gtk_widget_destroy(d -> btn);
}

void entry_callback(GtkEntry *entry, gpointer userdata) {
  const gchar *entry_text = gtk_entry_get_text(entry);
  printf("Entry contents: %s \n", entry_text);
  struct data *d = (struct data *)userdata;
  char *filename = d -> filename;

  if (check_if_file_exist(entry_text) == 0) {
    rename(filename, entry_text);
    gtk_label_set_text(GTK_LABEL(d -> label), entry_text);

    //Truncate file names that are too long
    char buf[35];
    if (strlen(entry_text) > 25) {
      strncpy(buf, entry_text, 24);
      strncat(buf, "...", 4);
    } else {
      strncpy(buf, entry_text, 25);
    }

    strncpy(d -> filename, entry_text, MAX_FILE_LEN);
    if (d -> called_from_popover == 1)
      gtk_popover_popdown(GTK_POPOVER(d -> popover));
  } else {
    GtkWidget *label = gtk_label_new("A file with that name already exists.");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_box_pack_start(GTK_BOX(d -> box2), label, FALSE, FALSE, 0);
    gtk_widget_show_all(d -> box2);
  }
}

void renamefile(GtkWidget *menuitem, gpointer userdata) {
  struct data *d = (struct data *)userdata;

  GtkWidget *entry = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(entry), MAX_FILE_LEN);
  gtk_entry_set_text(GTK_ENTRY(entry), d -> filename);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);

  GtkWidget *label = gtk_label_new(NULL);
  const char *format = "<b>File name</b>";
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_label_set_markup(GTK_LABEL(label), format);

  GtkWidget *box2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box2), 10);
  gtk_box_pack_start(GTK_BOX(box2), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box2), box, FALSE, FALSE, 0);
  d -> box2 = box2;

  GtkWidget *popover = gtk_popover_new(d -> btn);
  gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_BOTTOM);
  gtk_container_add(GTK_CONTAINER(popover), box2);
  gtk_popover_popup(GTK_POPOVER(popover));
  d -> popover = popover;
  d -> called_from_popover = 1;

  g_signal_connect(entry, "activate", G_CALLBACK(entry_callback), userdata);
  gtk_widget_show_all(popover);
}

void view_props(GtkWidget *menuitem, gpointer userdata) {
  struct data *d = (struct data *)userdata;
  char *filename = d -> filename;
  struct file_properties metadata = d -> metadata;

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), 500, 500);
  char buf[MAX_FILE_LEN + 15];
  strncpy(buf, filename, MAX_FILE_LEN);
  strncat(buf, " Properties", 15);
  gtk_window_set_title(GTK_WINDOW(window), buf);

  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
  gtk_container_set_border_width(GTK_CONTAINER(grid), 20);

  if (metadata.isdir == 0) {
    GtkWidget *namelabel = gtk_label_new("Name: ");
    gtk_label_set_xalign(GTK_LABEL(namelabel), 0.0);
    GtkWidget *sizelabel = gtk_label_new("Size: ");
    gtk_label_set_xalign(GTK_LABEL(sizelabel), 0.0);
    GtkWidget *parentfolder_label = gtk_label_new("location: ");
    gtk_label_set_xalign(GTK_LABEL(parentfolder_label), 0.0);
    GtkWidget *permslabel = gtk_label_new("Permissions: ");
    gtk_label_set_xalign(GTK_LABEL(permslabel), 0.0);
    GtkWidget *permslabel2 = gtk_label_new(metadata.perms);
    gtk_label_set_xalign(GTK_LABEL(permslabel2), 0.0);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), MAX_FILE_LEN);
    gtk_entry_set_text(GTK_ENTRY(entry), d -> filename);
    d -> called_from_popover = 0;
    g_signal_connect(entry, "activate", G_CALLBACK(entry_callback), userdata);

    char buf[30];
    if (metadata.size_bytes < 1024) {
      sprintf(buf, "%d", metadata.size_bytes);
      strncat(buf, " bytes", 7);
    } else {
      strncpy(buf, metadata.size, 10);
      strncat(buf, " (", 3);
      char buf2[20]; //lmk when you're browsing files greater than 19 digits of bytes in size
      sprintf(buf2, "%d", metadata.size_bytes);
      strncat(buf, buf2, 20);
      strncat(buf, " bytes", 7);
      strncat(buf, ")", 2);
    }

    GtkWidget *sizelabel2 = gtk_label_new(buf);
    gtk_label_set_xalign(GTK_LABEL(sizelabel2), 0.0);

    char cwd[100];
    getcwd(cwd, 100);
    GtkWidget *parentfolder_label2 = gtk_label_new(cwd);
    gtk_label_set_xalign(GTK_LABEL(parentfolder_label2), 0.0);

    gtk_grid_attach(GTK_GRID(grid), namelabel, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), sizelabel, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), sizelabel2, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), parentfolder_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), parentfolder_label2, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), permslabel, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), permslabel2, 1, 3, 1, 1);
  } else {
    GtkWidget *namelabel = gtk_label_new("Name: ");
    gtk_label_set_xalign(GTK_LABEL(namelabel), 0.0);
    GtkWidget *parentfolder_label = gtk_label_new("Parent folder: ");
    gtk_label_set_xalign(GTK_LABEL(parentfolder_label), 0.0);
    GtkWidget *permslabel = gtk_label_new("Permissions: ");
    gtk_label_set_xalign(GTK_LABEL(permslabel), 0.0);
    GtkWidget *permslabel2 = gtk_label_new(metadata.perms);
    gtk_label_set_xalign(GTK_LABEL(permslabel2), 0.0);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), MAX_FILE_LEN);
    gtk_entry_set_text(GTK_ENTRY(entry), d -> filename);
    d -> called_from_popover = 0;
    g_signal_connect(entry, "activate", G_CALLBACK(entry_callback), userdata);

    char cwd[100];
    getcwd(cwd, 100);
    GtkWidget *parentfolder_label2 = gtk_label_new(cwd);
    gtk_label_set_xalign(GTK_LABEL(parentfolder_label2), 0.0);

    gtk_grid_attach(GTK_GRID(grid), namelabel, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), parentfolder_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), parentfolder_label2, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), permslabel, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), permslabel2, 1, 3, 1, 1);
  }

  gtk_container_add(GTK_CONTAINER(window), grid);
  gtk_widget_show_all(window);
}

void createfile_callback(GtkEntry *entry, gpointer userdata) {
  struct data *d = malloc(sizeof(struct data));
  struct data *icondata = (struct data *)userdata;
  d -> grid = icondata -> grid;
  d -> col = icondata -> col;
  d -> row = icondata -> row;
  d -> box2 = icondata -> box2;
  d -> window = icondata -> window;

  GtkWidget *grid = d -> grid;
  int col = d -> col;
  int row = d -> row;

  const gchar *entry_text = gtk_entry_get_text(entry);
  printf("Entry contents: %s \n", entry_text);

  int newfile = new_file(entry_text);
  if (newfile == 1) {
    d -> isDir = 0;
    strncpy(d -> filename, entry_text, MAX_FILE_LEN);
    struct file_properties metadata;
    get_file_properties(entry_text, &metadata);
    d -> metadata = metadata;

    GtkWidget *iconbutton = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(iconbutton), GTK_RELIEF_NONE);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *iconimg = gtk_image_new_from_file(file_icon_image);

    GtkWidget *filename = gtk_label_new(entry_text);
    gtk_label_set_justify(GTK_LABEL(filename), GTK_JUSTIFY_CENTER);
    gtk_label_set_max_width_chars(GTK_LABEL(filename), 1);
    gtk_label_set_ellipsize(GTK_LABEL(filename), PANGO_ELLIPSIZE_END);
    d -> label = filename;

    gtk_box_pack_start(GTK_BOX(box), iconimg, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), filename, FALSE, FALSE, 0);

    g_signal_connect(iconbutton, "button_press_event", G_CALLBACK(btn_press), d);
    d -> btn = iconbutton;
    gtk_container_add(GTK_CONTAINER(iconbutton), box);

    if (col == 6) {
      row++;
      col = 0;
      gtk_grid_attach(GTK_GRID(grid), iconbutton, col, row, 1, 1);
      col++;
    } else
      gtk_grid_attach(GTK_GRID(grid), iconbutton, col++, row, 1, 1);


    icondata -> col = col;
    icondata -> row = row;
    d -> col = col;
    d -> row = row;
    gtk_widget_destroy(d -> window);
    gtk_widget_show_all(grid);
  } else {
    GtkWidget *label = gtk_label_new("A file/folder with that name already exists.");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_box_pack_start(GTK_BOX(d -> box2), label, FALSE, FALSE, 0);
    gtk_widget_show_all(d -> box2);
  }
}

void createfolder_callback(GtkEntry *entry, gpointer userdata) {
  struct data *d = malloc(sizeof(struct data));
  struct data *icondata = (struct data *)userdata;
  d -> grid = icondata -> grid;
  d -> col = icondata -> col;
  d -> row = icondata -> row;
  d -> box2 = icondata -> box2;
  d -> window = icondata -> window;

  GtkWidget *grid = d -> grid;
  int col = d -> col;
  int row = d -> row;

  const gchar *entry_text = gtk_entry_get_text(entry);
  printf("Entry contents: %s \n", entry_text);

  int newfolder = new_folder(entry_text);
  if (newfolder == 1) {
    d -> isDir = 1;
    strncpy(d -> filename, entry_text, MAX_FILE_LEN);
    struct file_properties metadata;
    get_file_properties (entry_text, &metadata);
    d -> metadata = metadata;

    GtkWidget *iconbutton = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(iconbutton), GTK_RELIEF_NONE);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *iconimg = gtk_image_new_from_file(folder_icon_image);

    GtkWidget *filename = gtk_label_new(entry_text);
    gtk_label_set_justify(GTK_LABEL(filename), GTK_JUSTIFY_CENTER);
    gtk_label_set_max_width_chars(GTK_LABEL(filename), 1);
    gtk_label_set_ellipsize(GTK_LABEL(filename), PANGO_ELLIPSIZE_END);
    d -> label = filename;

    gtk_box_pack_start(GTK_BOX(box), iconimg, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), filename, FALSE, FALSE, 0);

    g_signal_connect(iconbutton, "button_press_event", G_CALLBACK(btn_press), d);
    d -> btn = iconbutton;
    gtk_container_add(GTK_CONTAINER(iconbutton), box);

    if (col == 6) {
      row++;
      col = 0;
      gtk_grid_attach(GTK_GRID(grid), iconbutton, col, row, 1, 1);
      col++;
    } else
      gtk_grid_attach(GTK_GRID(grid), iconbutton, col++, row, 1, 1);

    icondata -> col = col;
    icondata -> row = row;
    d -> col = col;
    d -> row = row;
    gtk_widget_destroy(d -> window);
    gtk_widget_show_all(grid);
  } else {
    GtkWidget *label = gtk_label_new("A file/folder with that name already exists.");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_box_pack_start(GTK_BOX(d -> box2), label, FALSE, FALSE, 0);
    gtk_widget_show_all(d -> box2);
  }
}

void create_new_file(GtkWidget *menuitem, gpointer userdata) {
  struct data *d = (struct data *)userdata;
  GtkWidget *entry = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(entry), MAX_FILE_LEN);

  GtkWidget *label = gtk_label_new(NULL);
  const char *format = "<b>File name</b>";
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_label_set_markup(GTK_LABEL(label), format);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box), 10);
  gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  gtk_window_set_default_size(GTK_WINDOW(window), 350, 100);
  gtk_container_add(GTK_CONTAINER(window), box);
  d -> box2 = box;
  d -> window = window;

  g_signal_connect(entry, "activate", G_CALLBACK(createfile_callback), userdata);
  gtk_widget_show_all(window);
}

void create_new_folder(GtkWidget *newFolder, gpointer userdata) {
  struct data *d = (struct data *)userdata;
  GtkWidget *entry = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(entry), MAX_FILE_LEN);

  GtkWidget *label = gtk_label_new(NULL);
  const char *format = "<b>Folder name</b>";
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_label_set_markup(GTK_LABEL(label), format);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box), 10);
  gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);

  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  gtk_window_set_default_size(GTK_WINDOW(window), 350, 100);
  gtk_container_add(GTK_CONTAINER(window), box);
  d -> box2 = box;
  d -> window = window;

  g_signal_connect(entry, "activate", G_CALLBACK(createfolder_callback), userdata);
  gtk_widget_show_all(window);
}

void about_box(GtkWidget *about, gpointer userdata) {
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), 100, 100);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  gtk_window_set_title(GTK_WINDOW(window), "About");
  gtk_window_move(GTK_WINDOW(window), 380, 200);

  GtkWidget *label = gtk_label_new("This file explorer.\n Recreated by : Emmanuel Mwaba Mwangata,and Chalwe Silas \nwith reference to the attached documentation reference list ");
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);

  gtk_container_add(GTK_CONTAINER(window), label);
  gtk_widget_show_all(window);
}

gboolean btn_press(GtkWidget *btn, GdkEventButton *event, gpointer userdata) {
  if (event -> type == GDK_BUTTON_PRESS && event -> button == 3) { //right mouse button
    struct data *d = (struct data *)userdata;
    d -> btn = btn;

    GtkWidget *runfile = gtk_menu_item_new_with_label("Open");
    g_signal_connect(runfile, "activate", G_CALLBACK(executefile), userdata);

    GtkWidget *delete = gtk_menu_item_new_with_label("Delete");
    g_signal_connect(delete, "activate", G_CALLBACK(deletefile), userdata);

    GtkWidget *rename = gtk_menu_item_new_with_label("Rename...");
    g_signal_connect(rename, "activate", G_CALLBACK(renamefile), userdata);

    GtkWidget *viewprops = gtk_menu_item_new_with_label("Properties");
    g_signal_connect(viewprops, "activate", G_CALLBACK(view_props), userdata);

    GtkWidget *menu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), runfile);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), delete);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), rename);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), viewprops);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
    return TRUE;
  }

  else if (event -> type == GDK_2BUTTON_PRESS && event -> button == 1) { //double click
    struct data *d = (struct data *)userdata;
    if (d -> isDir == 0)
      Open_file(d -> filename);
    else {
      is_cd = 1;
      chdir(d -> filename);
      gtk_window_close(GTK_WINDOW(windoww));
      g_application_quit(G_APPLICATION(app));
}
    return TRUE;
  }

  return FALSE;
}

void back_press(GtkWidget *backbutton, gpointer userdata) {
  char cwd[100];
  getcwd(cwd, 100);
  if (strcmp("/home", cwd)){
    is_cd = 1;
    chdir("..");
    gtk_window_close(GTK_WINDOW(windoww));
    g_application_quit(G_APPLICATION(app));
  }
}

static void activate(GtkApplication *app, gpointer data) {
  windoww = gtk_application_window_new(app);
  gtk_window_set_default_size(GTK_WINDOW(windoww), 900, 500);
  gtk_window_set_resizable(GTK_WINDOW(windoww), FALSE);

  GtkWidget *titlebar;
  GtkWidget *grid;
  char cwd[100];
  getcwd(cwd, 100);
  GtkWidget *cwdlabel = gtk_label_new(cwd);

  titlebar = gtk_header_bar_new();
  gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(titlebar), TRUE);

  GtkWidget *separator = gtk_separator_menu_item_new();
  GtkWidget *newfile = gtk_menu_item_new_with_label("New File");
  GtkWidget *newfolder = gtk_menu_item_new_with_label("New Folder");
  GtkWidget *about = gtk_menu_item_new_with_label("About");

  GtkWidget *optionsmenu = gtk_menu_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(optionsmenu), newfile);
  gtk_menu_shell_append(GTK_MENU_SHELL(optionsmenu), newfolder);
  gtk_menu_shell_append(GTK_MENU_SHELL(optionsmenu), separator);
  gtk_menu_shell_append(GTK_MENU_SHELL(optionsmenu), about);

  g_signal_connect(about, "activate", G_CALLBACK(about_box), NULL);
  gtk_widget_show_all(optionsmenu);

  GtkWidget *menubutton = gtk_menu_button_new();
  gtk_menu_button_set_popup(GTK_MENU_BUTTON(menubutton), optionsmenu);
  gtk_menu_button_set_direction(GTK_MENU_BUTTON(menubutton), GTK_ARROW_DOWN);

  GtkWidget *backbutton = gtk_button_new_with_label("<");
  gtk_header_bar_pack_start(GTK_HEADER_BAR(titlebar), backbutton);
  g_signal_connect(backbutton, "button_press_event", G_CALLBACK(back_press), NULL);

  gtk_header_bar_pack_start(GTK_HEADER_BAR(titlebar), cwdlabel);
  gtk_header_bar_pack_end(GTK_HEADER_BAR(titlebar), menubutton);
  gtk_window_set_titlebar(GTK_WINDOW(windoww), titlebar);

  grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 50);
  gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(grid), 20);
  gtk_container_add(GTK_CONTAINER(windoww), GTK_WIDGET(grid));

  g_signal_connect(GTK_WINDOW(windoww), "key_press_event", G_CALLBACK(btn_press), NULL);

  int i;
  int row, col = 0;
  for (i = 0; i < num_files; i++) {
 //   printf("%s \n", files[i]);//show file
     //  printf("loading..... \n");
       //system("CLS");
    struct data * d = malloc(sizeof(struct data));
    strncpy(d -> filename, files[i], MAX_FILE_LEN);

    struct file_properties metadata;
    get_file_properties(files[i], &metadata);
    d -> metadata = metadata;

    GtkWidget *iconbutton = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(iconbutton), GTK_RELIEF_NONE); //gets rid of borders
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    if (metadata.isdir == 0) {
      GtkWidget *iconimg = gtk_image_new_from_file(file_icon_image);
      gtk_box_pack_start(GTK_BOX(box), iconimg, FALSE, FALSE, 0);
      d -> isDir = 0;
    } else {
      GtkWidget *folderimg = gtk_image_new_from_file(folder_icon_image);
      gtk_box_pack_start(GTK_BOX(box), folderimg, FALSE, FALSE, 0);
      d -> isDir = 1;
    }

    //Truncate file names that are too long
    char buf[35];
    if (strlen(files[i]) > 25) {
      strncpy(buf, files[i], 24);
      strncat(buf, "...", 4);
    } else {
      strncpy(buf, files[i], 25);
    }

    GtkWidget *filename = gtk_label_new(buf);
    gtk_label_set_justify(GTK_LABEL(filename), GTK_JUSTIFY_CENTER);
    gtk_label_set_max_width_chars(GTK_LABEL(filename), 1);
    gtk_label_set_ellipsize(GTK_LABEL(filename), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start(GTK_BOX(box), filename, FALSE, FALSE, 0);
    d -> label = filename;

    g_signal_connect(iconbutton, "button_press_event", G_CALLBACK(btn_press), d);
    gtk_container_add(GTK_CONTAINER(iconbutton), box);
    gtk_grid_attach(GTK_GRID(grid), iconbutton, col, row, 1, 1);

    col++;
    if (col == 6) {
      row++;
      col = 0;
    }
  }

  icon_location.grid = grid;
  icon_location.col = col;
  icon_location.row = row;
  g_signal_connect(newfile, "activate", G_CALLBACK(create_new_file), &icon_location);
  g_signal_connect(newfolder, "activate", G_CALLBACK(create_new_folder), &icon_location);

  gtk_widget_show_all(windoww);
}

int main(int argc, char *argv[]) {
  int status;
  while(0 < 1) {
    is_cd = 0;
    app = gtk_application_new(
      "EmmanuelMwabaMwangata.Chalwesilas.FileExplorer",
      G_APPLICATION_DEFAULT_FLAGS
    );
    num_files = 0;
    files = get_files(&num_files);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    free_files();
    g_object_unref(app);
    if(is_cd == 0) break; //x button breaks loop
  }
  return status;
}
