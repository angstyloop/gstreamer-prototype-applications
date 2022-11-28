/*
clear && gcc example.c -o example `pkg-config --cflags --libs gtk+-3.0 gstreamer-video-1.0` && ./example
 */

#include <gtk/gtk.h>
#include <gst/gst.h>

static GstElement *playbin, *sink;

static void play(GtkButton *play_button, gpointer user_data) {
    gst_element_set_state(playbin, GST_STATE_PLAYING);
}

static void activate(GtkApplication *app, gpointer user_data) {

    GtkWidget *window = gtk_application_window_new(app);

    GtkWidget *root_pane = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(window), root_pane);

    GtkWidget *video_drawing_area = gtk_drawing_area_new();

    g_object_get (sink, "widget", &video_drawing_area, NULL);
    gtk_widget_set_size_request(video_drawing_area, 800, 600);
    gtk_container_add(GTK_CONTAINER(root_pane), video_drawing_area);

    GtkWidget *play_button = gtk_button_new_with_label("Play");
    gtk_overlay_add_overlay(GTK_OVERLAY(root_pane), play_button);
    gtk_widget_set_halign(play_button, GTK_ALIGN_END);
    gtk_widget_set_valign(play_button, GTK_ALIGN_END);
    gtk_widget_set_margin_end(play_button, 20);
    gtk_widget_set_margin_bottom(play_button, 20);
    g_signal_connect(G_OBJECT(play_button), "clicked", G_CALLBACK(play), NULL);

    gtk_widget_show_all(window);

}

int main(int argc, char **argv) {

    gst_init(&argc, &argv);
    playbin = gst_element_factory_make("playbin", NULL);
    g_object_set(playbin, "uri", "http://techslides.com/demos/sample-videos/small.ogv", NULL);

    sink = gst_element_factory_make ("gtksink", NULL);
    g_object_set(playbin, "video-sink", sink, NULL);


    GtkApplication *app = gtk_application_new("com.techn.videotest", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);

}
