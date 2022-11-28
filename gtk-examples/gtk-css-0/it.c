/* FROM
https://stackoverflow.com/questions/47114306/why-is-css-style-not-being-applied-to-gtk-code
*/

/* COMPILE
clear && gcc -Wall -g -o it it.c `pkg-config --libs --cflags gtk+-3.0` && ./it
*/

#include <gtk/gtk.h>

static void
activate (GtkApplication *app, gpointer user_data) {
  GtkStyleContext *context;
  GtkWidget *button_01;
  GtkWidget *button_02;
  button_01 = gtk_button_new_with_label("This is a simple button");
  button_02 = gtk_button_new_with_label("This is a stylish button");

  context = gtk_widget_get_style_context (button_02);

  GtkCssProvider *provider = gtk_css_provider_new ();

  gtk_css_provider_load_from_path (provider,
    "it.css", NULL);

  GtkWidget *window;
  GtkWidget *box;

  window = gtk_application_window_new (app);
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 25);

  gtk_style_context_add_provider (context,
                                    GTK_STYLE_PROVIDER(provider),
                                    GTK_STYLE_PROVIDER_PRIORITY_USER);

  gtk_box_set_homogeneous (GTK_BOX (box), TRUE);
  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_container_add (GTK_CONTAINER (box), button_01);
  gtk_container_add (GTK_CONTAINER (box), button_02);

  gtk_widget_show_all (window);
}

int main (int argc, char **argv) {
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
