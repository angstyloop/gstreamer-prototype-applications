/*
gcc -Wall -o two-buttons-fail two_buttons_fail.c `pkg-config --libs --cflags gtk+-3.0`
*/

/* NOTES
 *
 * This program compiles, but produces a warning when run, because the window
 * can only have one widget as a direct child. See "two_buttons.c" for how
 * to to this properly using GtkBox.

(two-buttons-fail:11397): Gtk-WARNING **: 14:16:29.111: Attempting to add a widget with type GtkButton to a GtkWindow, but as a GtkBin subclass a GtkWindow can only contain one widget at a time; it already contains a widget of type GtkButton

*/

#include <gtk/gtk.h>

/** An "on click" callback for button 1
 */
static void button1_clicked() {
    g_print("clicked 2\n");
}

/** An "on click" callback for button 2
 */
static void button2_clicked() {
    g_print("clicked 2\n");
}

/** Application entrypoint.
 */
int main(int argc, char *argv[]) {
    // Initialize the object pointer variables we need.
    GtkWidget *window = NULL;
    GtkWidget *button1 = NULL, *button2 = NULL;

    // Start GTK
    gtk_init(&argc, &argv);

    // Create a GTK window.
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    // Set the window title.
    gtk_window_set_title(GTK_WINDOW(window), "GTK Window Title");
    // Size the window.
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 500);
    // Size the window border.
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    // Position the window.
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    // Attach a window callback.
    g_signal_connect(G_OBJECT(window), "destroy",
        G_CALLBACK(gtk_main_quit), NULL);

    // Create a GTK button.
    button1 = gtk_button_new_with_label("Click me!");
    // Size the button.
    gtk_widget_set_size_request(button1, 80, 30);
    // Position the button.
    gtk_widget_set_halign(button1, GTK_ALIGN_START);
    gtk_widget_set_valign(button1, GTK_ALIGN_START);
    // Add the button to the window.
    gtk_container_add(GTK_CONTAINER(window), button1);
    // Attach a button callback.
    g_signal_connect(G_OBJECT(button1), "clicked",
        G_CALLBACK(button1_clicked), NULL);

    // Create a GTK button.
    button2 = gtk_button_new_with_label("No, click me!");
    // Size the button.
    gtk_widget_set_size_request(button2, 110, 30);
    // Position the button.
    gtk_widget_set_halign(button2, GTK_ALIGN_END);
    gtk_widget_set_valign(button2, GTK_ALIGN_END);
    // Add the button to the window. This is where the warning is issued.
    gtk_container_add(GTK_CONTAINER(window), button2);
    // Attach a button callback.
    g_signal_connect(G_OBJECT(button2), "clicked",
        G_CALLBACK(button2_clicked), NULL);

    // Blit the window to the screen.
    gtk_widget_show_all(window);

    // Start the GTK main loop.
    gtk_main();

    return EXIT_SUCCESS;
}
