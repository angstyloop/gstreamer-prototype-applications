/*
gcc -Wall -o two-buttons two_buttons.c `pkg-config --libs --cflags gtk+-3.0`
*/

/** A GTK window can only show one widget at a time - what gives? How can you
 * have multiple buttons?
 *
 * You have to use a GtkBox. Add that element as the only direct child of the
 * window, and then pack it with all the elements you need.
 *
 * Another alternative is to use a grid, which we will explore in another
 * example.
 */

#include <gtk/gtk.h>

/** An "on click" callback for button 1
 */
static void button1_clicked() {
    g_print("clicked 1\n");
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
    GtkWidget *window, *button1, *button2, *vbox;

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

    // Create a vertical box to pack buttons into.
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Add the vbox to the window.
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Create a GTK button.
    button1 = gtk_button_new_with_label("Click me!");
    // Size the button.
    gtk_widget_set_size_request(button1, 80, 30);
    // Position the button.
    gtk_widget_set_halign(button1, GTK_ALIGN_START);
    gtk_widget_set_valign(button1, GTK_ALIGN_START);
    // Attach a button callback.
    g_signal_connect(G_OBJECT(button1), "clicked",
        G_CALLBACK(button1_clicked), NULL);
    // Pack the first button into the box.
    gtk_box_pack_start(GTK_BOX(vbox), button1, FALSE, FALSE, 10);

    // Create a GTK button.
    button2 = gtk_button_new_with_label("No, click me!");
    // Size the button.
    gtk_widget_set_size_request(button2, 110, 30);
    // Position the button.
    gtk_widget_set_halign(button2, GTK_ALIGN_START);
    gtk_widget_set_valign(button2, GTK_ALIGN_START);
    // Attach a button callback.
    g_signal_connect(G_OBJECT(button2), "clicked",
        G_CALLBACK(button2_clicked), NULL);
    // Pack the second button into the box.
    gtk_box_pack_start(GTK_BOX(vbox), button2, FALSE, FALSE, 10);

    // Show the window and all its children. You can show the widgets
    // individually, or use this convenience method.
    gtk_widget_show_all(window);

    // Start the GTK main loop.
    gtk_main();

    return EXIT_SUCCESS;
}
