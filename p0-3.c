/*
gcc p0-3.c -o p0-3 `pkg-config --cflags --libs gtk+-3.0 gstreamer-video-1.0`
*/

/* GTK/GStreamer example application with a dynamic pipeline. Click the button to
 * change the type of audiovisualizer element. The input source is a JACK Audio
 * Source.
 */

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include <gdk/gdk.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdkwin32.h>
#elif efined (GDK_WINDOWING_QUARTZ)
#include <gdk/gdkquartz.h>
#endif

#define DEFAULT_EFFECTS "spacescope,spectrascope,synaescope,wavescope"

static GstPad *blockpad;
static GstElement *sink;
static GstElement *conv_before;
static GstElement *conv_after;
static GstElement *cur_effect;
static GstElement *pipeline;
static GQueue effects = G_QUEUE_INIT;

/**
 * @brief button click handler
 * 
 * @param widget 
 * @param data 
 */

/**
 * @brief pad_probe_cb
 * 
 * this is where we add the block type probe and send an EOS to the source's sink pad 
 * 
 * from the Changing Elements in a Pipeline section of 
 * https://gstreamer.freedesktop.org/documentation/application-development/advanced/pipeline-manipulation.html?gi-language=c
 * 
 * @param pad 
 * @param info 
 * @param user_data 
 * @return GstPadProbeReturn 
 */

/**
 * @brief event_probe_cb
 * 
 * from the Changing Elements in a Pipeline section of 
 * https://gstreamer.freedesktop.org/documentation/application-development/advanced/pipeline-manipulation.html?gi-language=c
 * 
 * This is the callback that gets attached to the pad by gst_pad_add_probe() in
 * pad_probe_cb().
 * 
 * @param pad - (GstPad*)
 * @param info - (GstPadProbeInfo*)
 * @param user_data - (gpointer) this argument is unused but needs to be there.
 *
 * @return GstPadProbeReturn 
 */

/* This is "the block callback". */
static GstPadProbeReturn
event_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstElement *next;

  /* Pass until end of stream is reached */
  if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS)
    return GST_PAD_PROBE_PASS;

  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

  /* Push current effect back into the queue */
  g_queue_push_tail (&effects, gst_object_ref (cur_effect));

  /* Take next effect from the queue */
  next = g_queue_pop_head (&effects);

  /* If no effects remain, quit the main loop and drop the probe */
  if (next == NULL) {
    GST_DEBUG_OBJECT (pad, "no more effects");

    // No explicit "loop" argument, and shouldn't call gtk_main_quit() (a GTK
    // function) inside of a GStreamer callback, because callbacks execute in
    // the calling thread, which does not need to be the main thread. Thus
    // we push a "quit" message onto the bus.
    gst_element_post_message(pipeline,
        gst_message_new_application(GST_OBJECT (pipeline),
          gst_structure_new_empty("gtk-main-quit")));

    return GST_PAD_PROBE_DROP;
  }

  g_print ("Switching from '%s' to '%s'. \n", GST_OBJECT_NAME (cur_effect),
      GST_OBJECT_NAME (next));

  /* lower the state of the current element */
  gst_element_set_state (cur_effect, GST_STATE_NULL);

  /* Unlink and remove the current element. Note that
   * gst_bin_remove unlinks automatically
   */
  GST_DEBUG_OBJECT (pipeline, "removing %" GST_PTR_FORMAT, cur_effect);
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
 * @param info - (GstPadProbeInfo*)
 * @param user_data - (gpointer) this argument is unused but needs to be there.
 *
 * @return GstPadProbeReturn
 */
static GstPadProbeReturn
pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstPad *srcpad, *sinkpad;

  GST_DEBUG_OBJECT (pad, "pad is blocked now");

  /* Remove the probe first */
  gst_pad_remove_probe(pad, GST_PAD_PROBE_INFO_ID (info));

  /* Install new probe for EOS. This is the callback we defined above, event_probe_cb. */
  srcpad = gst_element_get_static_pad (cur_effect, "src");
  gst_pad_add_probe (srcpad, GST_PAD_PROBE_TYPE_BLOCK |
      GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, event_probe_cb, NULL, NULL);
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
 * now the button_clicked_ callback).
 */
static void
application_cb(GstBus *bus, GstMessage *msg)
{
  if (g_strcmp0 (gst_structure_get_name (gst_message_get_structure (msg)),
                 "gtk-main-quit") == 0) {

    gtk_main_quit();

  } else if (g_strcmp0 (gst_structure_get_name (gst_message_get_structure (msg)),
                                     "gtk-button-clicked") == 0) {

    // Dynamically change the effect element. The code isn't any different
    // here - we just took it out of timeout_cb
    gst_pad_add_probe (blockpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
      pad_probe_cb, NULL, NULL);

  }
}

/** Handle button click.
 */
void
button_clicked()
{
  // Instead of printing, we want to push an "application" message to the bus,
  // which will be parsed and handled in the main thread
  gst_element_post_message(pipeline,
      gst_message_new_application(GST_OBJECT (pipeline),
          gst_structure_new_empty("gtk-button-clicked")));
}

/** This is the primary primary callback function for the application.
 */
static void activate(GtkApplication *app, gpointer user_data) {

    GstElement *src, *q1, *q2, *effect;

    gchar **effect_names, **e;

    GtkWidget *window = gtk_application_window_new(app);

    GtkWidget *root_pane = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(window), root_pane);

    GtkWidget *video_drawing_area = gtk_drawing_area_new();

    g_object_get (sink, "widget", &video_drawing_area, NULL);
    gtk_widget_set_size_request(video_drawing_area, 800, 600);
    gtk_container_add(GTK_CONTAINER(root_pane), video_drawing_area);

    GtkWidget *btn = gtk_button_new_with_label("Click");
    gtk_overlay_add_overlay(GTK_OVERLAY(root_pane), btn);
    gtk_widget_set_size_request(btn, 70, 30);
    gtk_widget_set_halign(btn, GTK_ALIGN_START);
    gtk_widget_set_valign(btn, GTK_ALIGN_START);
    g_signal_connect(G_OBJECT(btn), "clicked",
        G_CALLBACK(button_clicked), NULL);

    gtk_widget_show_all(window);

      effect_names = g_strsplit (DEFAULT_EFFECTS, ",", -1);

      for (e = effect_names; e != NULL && *e != NULL; ++e) {
        GstElement *el;

        el = gst_element_factory_make (*e, NULL);
        if (el) {
          g_print ("Adding effect '%s'\n", *e);
          g_queue_push_tail (&effects, el);
        }
      }

      pipeline = gst_pipeline_new ("pipeline");

      src = gst_element_factory_make ("jackaudiosrc", NULL);
      g_object_set (src, "is-live", TRUE, NULL);

      q1 = gst_element_factory_make ("queue", NULL); 

      blockpad = gst_element_get_static_pad (q1, "src");

      conv_before = gst_element_factory_make ("audioconvert", NULL);

      effect = g_queue_pop_head (&effects);
      cur_effect = effect;

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
