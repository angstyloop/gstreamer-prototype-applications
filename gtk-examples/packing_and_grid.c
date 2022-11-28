/*
gcc `pkg-config --cflags gtk+-3.0` -o packing-and-grid packing_and_grid.c `pkg-config --libs gtk+-3.0`
*/

#include <gtk/gtk.h>

static void print_hello(GtkWidget* widget, gpointer data) {
    g_print("Hello world!\n");
}

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window;
    GtkWidget* grid;
    GtkWidget* button;

    // Create a new window, and set its title.
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "what");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    // Make a grid. We will pack elements into this.
    grid = gtk_grid_new();

    // Center the container.
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(grid, GTK_ALIGN_CENTER);

    // Set column and row spacing.
    gtk_grid_set_column_spacing(GTK_GRID(grid), 50);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 50);

    // Add the container to the window. The window can only contain one widget
    // as a direct descendant.
    gtk_container_add(GTK_CONTAINER(window), grid);

    // Create a button.
    button = gtk_button_new_with_label("wow");
    // Attach a click callback.
    g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL);

    // Place the first button in the grid cell (0, 0), and make it fill just 1
    // cell horizontally and vertically (i.e. no spanning).
    //gtk_grid_attach(GTK_GRID(grid), button, 0, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), button, 0, 0, 1, 1);

    // Create another button (re-using the same pointer variable - don't
    // worry, GTK will clean up for you!)
    button = gtk_button_new_with_label("oh");
    g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL);

    // Place the second button in the grid cell (1, 0), and make it fill just
    // 1 cell horizontally and vertically (i.e. no spanning).
    gtk_grid_attach(GTK_GRID(grid), button, 1, 0, 1, 1);

    button = gtk_button_new_with_label("byeee");
    g_signal_connect_swapped(button, "clicked", G_CALLBACK(gtk_widget_destroy), window);

    // Place the Quit button in the grid cell (0, 1), and make it span 2
    // columns.
    gtk_grid_attach(GTK_GRID(grid), button, 0, 1, 2, 1);

    // Show the window and all its widgets. 
    gtk_widget_show_all(window);
}

// Just like with every GTK application, we define a callback function that
// will be called when the application is ready, activate(). The activate()
// function contains all the actual application logic.
int main(int argc, char** argv) {
    int status = EXIT_FAILURE;
    GtkApplication* app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

/* REFERENCES
 * [0] https://developer-old.gnome.org/gtk3/stable/ch01s02.html
 */
