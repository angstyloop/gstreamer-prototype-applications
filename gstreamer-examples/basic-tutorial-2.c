/* RUN
clear && gcc basic-tutorial-2.c -o basic-tutorial-2 `pkg-config --cflags --libs gstreamer-1.0` && ./basic-tutorial-2
*/

/* CONCEPTS
 * - Define "GStreamer element"
 * - Create a GStreamer element.
 * - Customize an element's behavior.
 * - Link elements together.
 * - Watch the bus for error conditions and extract information from GStreamer
 *   messages.
 */ 

/* NOTES
 * gst_element_link_many, just like gst_bin_add_many, is a nice succint way to
 * link elements of linear pipelines.
 */

#include <gst/gst.h>

int main(int argc, char* argv[])
{
  GstElement* pipeline;
  GstElement* source;
  GstElement* effect;
  GstElement* convert;
  GstElement* sink;
  GstBus* bus;
  GstMessage* msg;
  GstStateChangeReturn ret;

  // Init GStreamer
  gst_init(&argc, &argv);

  // Create the elements
  source = gst_element_factory_make("videotestsrc", "source");
  effect = gst_element_factory_make("vertigotv", "effect"); 
  convert = gst_element_factory_make("videoconvert", "convert");
  sink = gst_element_factory_make("autovideosink", "sink");

  // Create an empty pipeline
  pipeline = gst_pipeline_new("test-pipeline");

  if( !pipeline || !source || !effect || !convert || !sink )
  {
    g_printerr("Not all elements could be created.\n");
    return EXIT_FAILURE;
  }

  // Build the pipeline

  gst_bin_add_many(GST_BIN(pipeline), source, effect, convert, sink, NULL);

  if( gst_element_link_many(source, effect, convert, sink, NULL) != TRUE )
  {
    g_printerr("Elements could not all be linked: source -> effect -> convert -> sink\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  // Modify the source's properties
  g_object_set(source, "pattern", 0, NULL);

  // Start playing
  ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if( ret==GST_STATE_CHANGE_FAILURE )
  {
    g_printerr("Unable to set the pipeline to the playing state.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  // Create a bus
  bus = gst_element_get_bus(pipeline);

  // Wait until error or EOS. This is synchronous. For asynchronous extraction,
  // you can use signals, as seen in the next example.
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  // Parse message
  if( !msg )
  {
    GError* err;
    gchar* debug_info;

    switch( GST_MESSAGE_TYPE(msg) )
    {
      // Error
      case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &debug_info);

        g_printerr("Error received from element %s: %s\n.",
            GST_OBJECT_NAME(msg->src), err->message);

        g_printerr("Debugging information: %s\n",
            debug_info ? debug_info : "none");

        g_clear_error(&err);
        g_free(debug_info);
        break;

      // End of stream
      case GST_MESSAGE_EOS:
        g_print("End-Of-Stream reached.\n");
        break;

      default:
        // Should never reach here, because we filtered the events to EOS and
        // ERROR in gst_bus_timed_pop_filtered
        g_printerr("Unexpected message received");
    }
  }

  // GC
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return EXIT_SUCCESS;
}
