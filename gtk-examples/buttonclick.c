/*
clear && gcc -Wall -o buttonclick buttonclick.c `pkg-config --libs --cflags gtk+-3.0` && ./buttonclick
*/

#include <gtk/gtk.h>

void button_clicked() {
  g_print("clicked\n");
}

int main(int argc, char *argv[]) {
  GtkWidget *window;
  GtkWidget *btn;

  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "GtkButton");
  gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
  gtk_container_set_border_width(GTK_CONTAINER(window), 15);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

  btn = gtk_button_new_with_label("Click");
  gtk_widget_set_size_request(btn, 70, 30);

  gtk_widget_set_halign(btn, GTK_ALIGN_START);
  gtk_widget_set_valign(btn, GTK_ALIGN_START);

  gtk_container_add(GTK_CONTAINER(window), btn);

  g_signal_connect(G_OBJECT(btn), "clicked",
      G_CALLBACK(button_clicked), NULL);

  g_signal_connect(G_OBJECT(window), "destroy",
      G_CALLBACK(gtk_main_quit), NULL);

  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}
