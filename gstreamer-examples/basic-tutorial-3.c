/*
clear && gcc basic-tutorial-3.c -o basic-tutorial-3 `pkg-config --cflags --libs gstreamer-1.0` && ./basic-tutorial-3
*/

#include<gst/gst.h>

/* CONCEPTS
 * - Receive notifications using GSignals
 * - Connect GstPad objects directly instead of their parent elements
 * - The states of a GStreamer element 
 */

// Struct to contain data to be passed to callbacks
typedef struct _CustomData
{
  GstElement* pipeline;
  GstElement* source;
  GstElement* audioconvert;
  GstElement* resample;
  GstElement* audiosink;
  GstElement* videoconvert;
  GstElement* videosink;
} CustomData;

// Handler for the pad-added signal.
static void pad_added_handler(GstElement* src, GstPad* pad, CustomData* data);

int main(int argc, char* argv[])
{
  CustomData data;
  GstBus* bus;
  GstMessage* msg;
  GstStateChangeReturn ret;

  // Controls the loop where we listen for signals on the bus
  gboolean terminate = FALSE;

  // Init GStreamer
  gst_init(&argc, &argv);

  // Create elements
  data.source = gst_element_factory_make("uridecodebin", "source");

  data.audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
  data.resample = gst_element_factory_make("audioresample", "resample");
  data.audiosink = gst_element_factory_make("autoaudiosink", "audiosink");

  data.videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  data.videosink = gst_element_factory_make("autovideosink", "videosink");

  // Create the empty pipeline
  data.pipeline = gst_pipeline_new("test-pipeline");

  if( !data.pipeline || !data.source || !data.audioconvert || !data.resample
      || !data.audiosink || !data.videoconvert || !data.videosink )
  {
    g_printerr("Not all elements could be created.\n");
    return EXIT_FAILURE;
  }

  // Add audio elements to the pipeline.
  gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.audioconvert,
      data.resample, data.audiosink, NULL);

  // Link the audio elements together, but don't link to the source. Save that
  // for the pad-added handler.
  if( !gst_element_link_many(data.audioconvert, data.resample, data.audiosink, NULL) )
  {
    g_printerr("Audio elements could not be linked.\n");
    gst_object_unref(data.pipeline);
    return EXIT_FAILURE;
  }

  // Add video elements to the pipeline.
  gst_bin_add_many(GST_BIN(data.pipeline), data.videoconvert, data.videosink, NULL);

  // Link the video elements together, but don't link to the source. Save that
  // for the pad-added handler. 
  if( !gst_element_link_many(data.videoconvert, data.videosink, NULL) )
  {
    g_printerr("Video elements could not be linked.\n");
    gst_object_unref(data.pipeline);
    return EXIT_FAILURE;
  }

  // Set the URI to play
  g_object_set(data.source, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);

  // Connect to the pad-added signal. This makes it so that the
  // pad_added_handler function is called when a pad-added signal is received.
  g_signal_connect(data.source, "pad-added", G_CALLBACK(pad_added_handler), &data);

  // Start playing
  ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

  // State change is delicate - always check for failure using a
  // GstStateChangeReturn object.
  if( ret==GST_STATE_CHANGE_FAILURE )
  {
    g_printerr("Unable to set the pipeline to the playing state.\n");
    gst_object_unref(data.pipeline);
    return EXIT_FAILURE;
  }

  // Get the bus
  bus = gst_element_get_bus(data.pipeline);

  // Listen to the bus
  do
  {
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    // Parse message
    if( msg != NULL )
    {
      GError* err;
      gchar* debug_info;

      switch(GST_MESSAGE_TYPE(msg))
      {
        case GST_MESSAGE_ERROR:
          gst_message_parse_error(msg, &err, &debug_info);
          g_printerr("Error received from element %s: %s\n",
              GST_OBJECT_NAME(msg->src), err->message);

          g_printerr("Debugging information: %s\n",
              debug_info ? debug_info : "none");

          g_clear_error(&err);
          g_free(debug_info);
          terminate = TRUE;
          break;

        case GST_MESSAGE_EOS:
          g_print("End-Of-Stream reached.\n");
          terminate = TRUE;
          break;

        case GST_MESSAGE_STATE_CHANGED:
          // We are only interested in state-changed messages from the pipeline
          if(GST_MESSAGE_SRC(msg) == GST_OBJECT(data.pipeline))
          {
            GstState old_state;
            GstState new_state;
            GstState pending_state;
            gst_message_parse_state_changed(msg, &old_state, &new_state,
                &pending_state);
            g_print("Pipeline state changed from %s to %s\n",
                gst_element_state_get_name(old_state),
                gst_element_state_get_name(new_state));
          }
          break;

        default:
          // Should not reach here, since we only specified the above three
          // events in gst_bus_timed_pop_filtered
          g_printerr("Unexpected message received.\n");
          break;
      }
      gst_message_unref(msg);
    }
  }
  while (!terminate);

  // Clean up
  gst_object_unref(bus);
  gst_element_set_state(data.pipeline, GST_STATE_NULL);
  gst_object_unref(data.pipeline);

  return EXIT_SUCCESS;
}

static void pad_added_handler(GstElement* src,
                              GstPad* new_pad,
                              CustomData* data)
{
  GstPadLinkReturn ret;
  GstCaps* new_pad_caps = NULL;
  GstStructure* new_pad_struct = NULL;
  const gchar* new_pad_type = NULL;

  g_print("Received new pad '%s' from '%s'\n",
      GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

  // Check the new pad's type
  new_pad_caps = gst_pad_get_current_caps(new_pad);
  new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
  new_pad_type = gst_structure_get_name(new_pad_struct);
  if( g_str_has_prefix(new_pad_type, "audio/x-raw") )
  {
    // Get the "sink" pad of the audioconvert element. Every audioconvert
    // element has a pad named "sink" and a pad named "src". 
    GstPad* audiosinkpad = gst_element_get_static_pad(data->audioconvert, "sink");
    // If already linked, there's nothing to do.
    if ( gst_pad_is_linked(audiosinkpad) )
    {
      goto exit;
    }
    // Otherwise, attempt the link
    ret = gst_pad_link(new_pad, audiosinkpad);
  }
  else if( g_str_has_prefix(new_pad_type, "video/x-raw") )
  {
    // Get the "sink" pad of the videoconvert element. Every videoconvert
    // element has a pad named "sink" and a pad named "src". 
    GstPad* videosinkpad = gst_element_get_static_pad(data->videoconvert, "sink");
    // If already linked, there's nothing to do.
    if( gst_pad_is_linked(videosinkpad) )
    {
      goto exit;
    }
    // Otherwise, attempt the link
    ret = gst_pad_link(new_pad, videosinkpad);
  }
  else
  {
    g_print("It has type '%s' which is neither raw audio nor raw video. Ignoring.\n",
        new_pad_type);
    goto exit;
  }

  // Check the link succeeded
  if( GST_PAD_LINK_FAILED(ret) )
  {
    g_print("Type is '%s', but link failed.\n", new_pad_type);
  }
  else
  {
    g_print("Link succeeded(type '%s').\n", new_pad_type);
  }

// This is a label, if you're new to C. You can jump to them with the goto
// keyword. This is one of the few accepted uses of it - having a common
// place for cleanup.
exit:
  // Unreference the new pad's caps (capabilities) if we got them
  if( new_pad_caps!=NULL )
  {
    gst_caps_unref(new_pad_caps);
  }

  // Unref the sink pads
  gst_object_unref(audiosinkpad);
  gst_object_unref(videosinkpad);

}
