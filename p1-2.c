/*
clear && gcc -g p1-1.c -o p1-1 `pkg-config --cflags --libs gtk+-3.0 gstreamer-video-1.0` && ./p1-1
*/

/* GTK/GStreamer example application with a dynamic pipeline. Click the buttons
 * to select from four types of audiovisualizer element. The input source is a
 * JACK Audio Source.
 */

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include <assert.h> // <0.o> This program will fail due to assertions.

#include <gdk/gdk.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdkwin32.h>
#elif defined (GDK_WINDOWING_QUARTZ)
#include <gdk/gdkquartz.h>
#endif

static GstPad* blockpad;
static GstElement* sink;
static GstElement* conv_before;
static GstElement* conv_after;
static GstElement* cur_effect;
static GstElement* pipeline;
static GstElement* effects[] = { NULL, NULL, NULL, NULL, NULL };

/**
 * @brief event_probe_cb
 * 
 * from the Changing Elements in a Pipeline section of 
 * https://gstreamer.freedesktop.org/documentation/application-development/advanced/pipeline-manipulation.html?gi-language=c
 * 
 */
static GstPadProbeReturn
event_probe_cb (GstPad* pad, GstPadProbeInfo* info, GstStructure* data)
{
  // Pass until end of stream is reached
  if (GST_EVENT_TYPE(GST_PAD_PROBE_INFO_DATA(info)) != GST_EVENT_EOS) {
    return GST_PAD_PROBE_PASS;
  }

  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID(info));

  // Unlike gst_structure_get_string, GStreamer's gst_structure_get_int
  // function needs you to provide the address of a variable where
  // gst_structure_get_int can write the value. This is simply so that
  // GStreamer has a way to tell you if there was an error. The
  // gst_structure_get_int function does this by returning TRUE on
  // success and FALSE on failure (TRUE and FALSE are GObject macros for
  // objects of type gboolean);
  gint index=0;
  gboolean pass = gst_structure_get_int(data, "index", &index);
  if (!pass) {
    // In C, adjacent string literals get concatenated, and all whitespace
    // between adjacent string literals is ignored.
    g_print("\nThere is no field with name \"index\", or the existing field "
            "does not contain an integer.");
  }

  if (index != 4) {
    g_print("Button clicked (index=%d)", index);
  }
  // TODO otherwise
  //     get the @id property out of @data
  //     set @next equal to @effects[@id]
  //     proceed

  // TODO if the @index property of @data is 4, post a quit message to the
  //      bus
  else {
    GST_DEBUG_OBJECT (pad, "Quit Button click event received.");

    // Push a "quit" message onto the bus.
    gst_element_post_message(pipeline,
        gst_message_new_application(GST_OBJECT (pipeline),
            gst_structure_new_empty("gtk-main-quit")));

    return GST_PAD_PROBE_DROP;
  }

  assert(cur_effect);

  // Define a variable to hold the next audiovisualizer GStreamer pipeline
  // element pointer. Initialize it to the current effect element.
  GstElement* next = effects[index];
  
  assert(next);

  g_print ("Switching from '%s' to '%s'. \n", GST_OBJECT_NAME (cur_effect),
      GST_OBJECT_NAME (next));

  /* lower the state of the current element */
  gst_element_set_state (cur_effect, GST_STATE_NULL);

  // Unlink and remove the current element, but avoid dereferencing it. See
  // below NOTES.
  //
  // NOTES on gst_bin_remove
  //
  // FROM https://gstreamer.freedesktop.org/documentation/gstreamer/gstbin.html?gi-language=c
  //
  // """
  // Removes the element from the bin, unparenting it as well. Unparenting the
  // element means that the element will be dereferenced, so if the bin holds
  // the only reference to the element, the element will be freed in the process
  // of removing it from the bin. If you want the element to still exist after
  // removing, you need to call gst_object_ref before removing it from the
  // bin.
  // """
  GST_DEBUG_OBJECT (pipeline, "removing %" GST_PTR_FORMAT, cur_effect);
  gst_object_ref(cur_effect);
  gst_bin_remove (GST_BIN (pipeline), cur_effect);

  /* Add the next element to the pipeline. */
  GST_DEBUG_OBJECT (pipeline, "adding... %" GST_PTR_FORMAT, next);
  gst_bin_add (GST_BIN (pipeline), next);

  /* Link the new element to the appropriate elements. */
  GST_DEBUG_OBJECT (pipeline, "linking...");
  gst_element_link_many (conv_before, next, conv_after, NULL);

  gst_element_set_state (next, GST_STATE_PLAYING);

  cur_effect = next;

  GST_DEBUG_OBJECT (pipeline, "done");

  /* Drop the probe */
  return GST_PAD_PROBE_DROP;
}

/* This is the callback function responsible for adding and removing the
 * event_probe_cb, in order to facilitate the dynamic addition or removal of
 * GStreamer pipeline elements.
 *
 * @param pad - (GstPad*)
 *
 * @param info - (GstPadProbeInfo*)
 *
 * @param data - (gpointer) this argument holds custom data we want
 *               passed to event_probe_cb 
 *
 * @return GstPadProbeReturn
 */
static GstPadProbeReturn
pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, GstStructure* data)
{
  GstPad *srcpad, *sinkpad;

  GST_DEBUG_OBJECT (pad, "pad is blocked now");

  /* Remove the probe first (so it doesn't keep firing) */
  gst_pad_remove_probe(pad, GST_PAD_PROBE_INFO_ID (info));

  /* Install new probe for EOS. This is the callback we defined above, event_probe_cb. */
  srcpad = gst_element_get_static_pad (cur_effect, "src");
  gst_pad_add_probe (srcpad, GST_PAD_PROBE_TYPE_BLOCK |
      GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, (GstPadProbeCallback)event_probe_cb, data, NULL);
  gst_object_unref (srcpad);

  /* Push EOS into the element. The probe will be fired when EOS leaves the
   * effect element, at which point all the data has been drained.
   */
  sinkpad = gst_element_get_static_pad (cur_effect, "sink");
  gst_pad_send_event(sinkpad, gst_event_new_eos());
  gst_object_unref (sinkpad);

  return GST_PAD_PROBE_OK;
}

/* This function is called when an error message is posted on the bus.
 * @param bus - (GstBus*)
 * @param msg - (GstMessage*)
 */
static void
error_cb (GstBus *bus, GstMessage *msg)
{
  GError *err = NULL;
  gchar *dbg;

  gst_message_parse_error (msg, &err, &dbg);
  gst_object_default_error (msg->src, err, dbg);
  g_clear_error (&err);
  g_free (dbg);

  // No explicit "loop" argument, and shouldn't call gtk_main_quit() (a GTK
  // function) inside of a GStreamer callback, because callbacks execute in
  // the calling thread, which does not need to be the main thread. Thus
  // we push a "quit" message onto the bus.
    
  gst_element_post_message(pipeline,
      gst_message_new_application(GST_OBJECT (pipeline),
        gst_structure_new_empty("gtk-main-quit")));
}

/* This callback function is run in the main thread, so it's safe to use GTK
 * functions. This function is called when an "application" message is posted on
 * the bus. Here we retrieve the message posted by the tags_cb callback (and
 * now the button_clicked callback).
 */
static void
application_cb(GstBus *bus, GstMessage *msg)
{
  // Get the GstStructure out of the GstMessage.
  GstStructure* data = gst_structure_copy(gst_message_get_structure(msg));

  // Get the name we gave the GstStructure.
  gchar const*const  name = gst_structure_get_name(data);

  if (!g_strcmp0(name, "gtk-main-quit")) {
    // If we named the structure "gtk-main-quit", quit our application by
    // quittingGTK, which will also take down GSstreamer.
    //
    // This will cause an error message to be printed as GStreamer goes down.
    gtk_main_quit();
  } else if (!g_strcmp0(name, "gtk-button-clicked")) {
    // Set up a GStreamer Pad Probe, which will safely stop streaming,
    // dynamically swap out the GStreamer audiovisualizer element.
    gst_pad_add_probe (blockpad,
                       GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
                       (GstPadProbeCallback)pad_probe_cb,
                       data,
                       NULL);
  }
}

/**
 * @brief A button click handler. posts a message containing the button index
 *        to the GstBus.
 * 
 * @param widget {GtkWidget} - This GtkWidget is connected to the signal
 *               handler.
 *
 * @param i {gpointer} - A pointer to the button index (gint).
 */
void
button_clicked(GtkWidget* widget, gint* index_ptr)
{
  // Create a GstStructure @data containing the button index.
  GstStructure* gst_struct =
      gst_structure_new("gtk-button-clicked",
                        "index", G_TYPE_INT, *index_ptr,
                        NULL);

  int i=0;
  gst_structure_get_int(gst_struct, "index", &i);

  // Create a GstMessage @msg containing the GstStructure @gst_struct.
  GstMessage* msg =
      gst_message_new_application(GST_OBJECT(pipeline), gst_struct);

  // Push an application message to the bus. The message will be parsed and
  // handled by application_cb, which runs in the main thread.
  gst_element_post_message(pipeline, msg);
}

/** This is the primary primary callback function for the application.
 */
static void
activate(GtkApplication *app, gpointer _data/*gpointer data*/)
{
  GstElement *src, *q1, *q2, *effect;

  GtkWidget *window = gtk_application_window_new(app);

  GtkWidget* grid = gtk_grid_new();

  // Set the space between Grid columns (in pixels)
  gtk_grid_set_column_spacing(GTK_GRID(grid), 20);

  gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(grid, GTK_ALIGN_CENTER);

  GtkWidget *root_pane = gtk_overlay_new();
  gtk_container_add(GTK_CONTAINER(window), grid);

  GtkWidget *video_drawing_area = gtk_drawing_area_new();

  g_object_get (sink, "widget", &video_drawing_area, NULL);
  gtk_widget_set_size_request(video_drawing_area, 800, 600);
  gtk_grid_attach(GTK_GRID(grid), video_drawing_area, 2, 0, 3, 5);

  // Create the buttons
  GtkWidget* buttons[4] = {
    gtk_button_new_with_label("spacescope"),
    gtk_button_new_with_label("spectrascope"),
    gtk_button_new_with_label("synaescope"),
    gtk_button_new_with_label("wavescope"),
  };

  // Create containers for the buttons
  GtkWidget* boxes[4] = {
    gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12),
    gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12),
    gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12),
    gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12),
  };

  GtkStyleContext* context = gtk_widget_get_style_context(button);
  GtkCssProvider* provider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(provider, "button.css", NULL);

  // Define the @button_indices array.
  gint indices[4] = { 0, 1, 2, 3 };

  // Define the @effect_names array.
  gchar* effect_names[4] = {
    "spacescope",
    "spectrascope",
    "synaescope",
    "wavescope"
  };

  // { HORIZONTAL, VERTICAL }
  gint coords[4][2] = {
    {0, 2},
    {1, 2},
    {0, 3},
    {1, 3},
  };

  // For each effect button...
  for (unsigned i = 0; i < 4; ++i) {
    // Connect the `button_clicked` function to the "clicked" signal
    // on the button.
    //
    // (This is where the button click handler is attached.)
    g_signal_connect(G_OBJECT(buttons[i]), "clicked",
        G_CALLBACK(button_clicked), &indices[i]);

    // Initialize the gst element corresponding to the button.
    effects[i] = gst_element_factory_make(effect_names[i], NULL);

    // Add the button to its container.
    gtk_box_pack_end(GTK_BOX(boxes[i]), buttons[i], TRUE, TRUE, 0);

    // Position the container with Grid.
    gtk_grid_attach(GTK_GRID(grid), boxes[i], coords[i][0], coords[i][1], 1, 1);

  }

  g_object_set(buttons[0], "margin-top", 220, NULL);
  g_object_set(buttons[0], "margin-bottom", 20, NULL);
  g_object_set(buttons[0], "margin-left", 20, NULL);

  g_object_set(buttons[1], "margin-top", 220, NULL);
  g_object_set(buttons[1], "margin-bottom", 20, NULL);

  g_object_set(buttons[2], "margin-bottom", 220, NULL);
  g_object_set(buttons[2], "margin-left", 20, NULL);

  g_object_set(buttons[3], "margin-bottom", 220, NULL);


  // Recursively show window and all its children.
  gtk_widget_show_all(window);

  pipeline = gst_pipeline_new ("pipeline");

  // Create the source element for the pipeline, a Jack Audio Source.
  src = gst_element_factory_make ("jackaudiosrc", NULL);

  // The jackaudiosrc element doesn't have this.
  //g_object_set (src, "is-live", TRUE, NULL);

  q1 = gst_element_factory_make ("queue", NULL); 

  blockpad = gst_element_get_static_pad (q1, "src");

  conv_before = gst_element_factory_make ("audioconvert", NULL);

  // The default effect is the first one in the @effects array.
  cur_effect = effect = effects[0];

  conv_after = gst_element_factory_make ("videoconvert", NULL);

  q2 = gst_element_factory_make ("queue", NULL);

  gst_bin_add_many (GST_BIN (pipeline), src, q1, conv_before, effect,
      conv_after, q2, sink, NULL);

  gst_element_link_many (src, q1, conv_before, effect,
      conv_after, q2, sink, NULL);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  GstBus *bus = gst_element_get_bus (pipeline);
  gst_bus_add_signal_watch (bus);
  g_signal_connect (G_OBJECT (bus),
                    "message::error",
                    (GCallback)error_cb,
                    NULL);
  g_signal_connect (G_OBJECT (bus),
                    "message::application",
                    (GCallback)application_cb,
                    NULL);

  gtk_main();

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
}

/** This main function sets up the activate callback and then starts the
 * application. The activate callback will be called when the application is
 * ready.
 */
int main(int argc, char **argv) {
    gst_init(&argc, &argv);
    sink = gst_element_factory_make ("gtksink", NULL);
    GtkApplication *app = gtk_application_new("com.gst.proto", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}
