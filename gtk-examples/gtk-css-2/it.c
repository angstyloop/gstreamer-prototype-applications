/* FROM
../gtk-css-1
*/

/* COMPILE
clear && gcc -Wall -g -o it it.c `pkg-config --libs --cflags gtk+-3.0` && ./it
*/

#include <gtk/gtk.h>

static void
activate (GtkApplication *app, gpointer user_data) {
  GtkStyleContext *context;
  GtkWidget *button;
  button = gtk_button_new_with_label("SAVE CHANGES");
  gtk_widget_set_name(button, "it");
  gtk_widget_set_size_request(button, 100, 50);
  gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(button, GTK_ALIGN_CENTER);

  context = gtk_widget_get_style_context (button);

  GtkCssProvider *provider = gtk_css_provider_new ();

  gtk_css_provider_load_from_path (provider, "it.css", NULL);

  GtkWidget *window;

  window = gtk_application_window_new (app);
  gtk_window_set_default_size(GTK_WINDOW(window), 500, 500);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

  gtk_container_add(GTK_CONTAINER(window), button);

  gtk_style_context_add_provider (context,
                                  GTK_STYLE_PROVIDER(provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_USER);

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
