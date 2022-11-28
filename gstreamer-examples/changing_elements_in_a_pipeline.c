/*

!! WIP

clear && gcc -o changing-elements-in-a-pipeline changing_elements_in_a_pipeline.c `pkg-config --cflags --libs gstreamer-1.0 gstreamer-audio-1.0 gstreamer-video-1.0` && ./changing-elements-in-a-pipeline

https://gstreamer.freedesktop.org/documentation/application-development/advanced/pipeline-manipulation.html?gi-language=c

In this example we have the following element chain:

    - ----.     .----------.       .---- -
element 1 |     | element2 |       | element3
        src -> sink      src -> sink
    - ----.     .----------.       .---- -

We want to replace element 2 by element4 while the pipeline is in the PLAYING
state. Let's say that element2 is a visualization and that you want to switch
the visualization in the pipeline. 

We can't just unlink element2's sinkpad from element1's source pad because that
would leave element1's source pad unlinkekd and would cause a streaming error
int he pipeline when data is pushed on the source pad. The technique is to block
the dataflow from element1's source pad before we replace element2 by element4
and then resume dataflow as shown in the following steps:
  
  - Block element1's source pad with a blocking pad probe. When the pad is
    blocked, the probe callback will be called. 

  - Inside the block callback nothing is flowing between element1 and element2
    and nothing will flow until unblocked.

  - Unlink element1 and element2

  - Make sure data is flushed out of element2. Some elements might internally
    keep some data, so you need to make sure not to lose any by forcing it out
    of element2. You can do this by pushing EOS into the element2, like this:

      - Put an event probe on element2's source pad.

      - Send EOS to element2's sink pad. This makes sure that all the data
        inside element2 is forced out.

      - Wait for the EOS event to appear on element2's source pad. When the EOS
        is received, drop it and remove the event probe.

  - Unlink element2 and element3. you can now also remove element2 from the
    pipeline and set the state to NULL.

  - Add element 4 to the pipleine, if not already added. Link element4 and
    element3. Link element 1 and element4.

  - Make sure element4 is in the same state as the rest of the elements in the
    pipeline. It should be at least in the PAUSED state before it can receive
    buffers and events.

  - Unblock element1's source pad probe. This will let new data into element4
    and continue streaming

The above algorithm works when the source pad is blocked, i.e.,w hen there is
dataflow in the pipeline. If there is no dataflow, there is also no point in
changing the element (just yet) so this algorithm can be used in the PAUSED
state as well.

This example changes the video effect on a simple pipeline once per second.

*/

#include <gst/gst.h>

static gchar *opt_effects = NULL;

#define DEFAULT_EFFECTS "identity,exclusion,navigationtest," \
    "agingtv,videoflip,vertigotv,gaussianblur,shagadelictv,edgetv"

static GstPad *blockpad;
static GstElement *conv_before;
static GstElement *conv_after;
static GstElement *cur_effect;
static GstElement *pipeline;

static GQueue effects = G_QUEUE_INIT;

/* This is "the block callback". */
static GstPadProbeReturn
event_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GMainLoop *loop = user_data;
  GstElement *next;

  /* Pass until end of stream is reached */
  if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS)
    return GST_PAD_PROBE_PASS;

  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

  /* push current effect back into the queue */
  g_queue_push_tail (&effects, gst_object_ref (cur_effect));

  /* take next effect from the queue */
  next = g_queue_pop_head (&effects);

  /* if no effects remain, quit the main loop and drop the probe */
  if (next == NULL) {
    GST_DEBUG_OBJECT (pad, "no more effects");
    g_main_loop_quit (loop);
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
      GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, event_probe_cb, user_data, NULL);
  gst_object_unref (srcpad);

  /* Push EOS into the element. The probe will be fired when EOS leaves the
   * effect element, at which point all the data has been drained.
   */
  sinkpad = gst_element_get_static_pad (cur_effect, "sink");
  gst_pad_send_event(sinkpad, gst_event_new_eos());
  gst_object_unref (sinkpad);

  return GST_PAD_PROBE_OK;
}

/* This function adds the callback function we just defined to the blocking
 * pad. It is presumably called asynchronously with a "set timeout" function
 */
static gboolean
timeout_cb (gpointer user_data)
{
  gst_pad_add_probe (blockpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
      pad_probe_cb, user_data, NULL);

  return TRUE;
}

static gboolean  
bus_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GMainLoop *loop = user_data;

  /* Right now we are only checking for error messages. We want to modify this
   * later to check for messages from GTK UI elements. */
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;
      gchar *dbg;

      gst_message_parse_error (msg, &err, &dbg);
      gst_object_default_error (msg->src, err, dbg);
      g_clear_error (&err);
      g_free (dbg);
      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }
  return TRUE;
}

int
main (int argc, char **argv)
{
  GOptionEntry options[] = {
    {"effects", 'e', 0, G_OPTION_ARG_STRING, &opt_effects,
        "Effects to use (comma-separated list of element names)", NULL},
    {NULL}
  };
  GOptionContext *ctx;
  GError *err = NULL;
  GMainLoop *loop;
  GstElement *src, *q1, *q2, *effect, *filter1, *filter2, *sink;
  gchar **effect_names, **e;

  ctx = g_option_context_new("");
  g_option_context_add_main_entries (ctx, options, NULL);
  g_option_context_add_group (ctx, gst_init_get_option_group ());
  if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
    g_print ("Error initializing: %s\n", err->message);
    g_clear_error (&err);
    g_option_context_free (ctx);
    return 1;
  }
  g_option_context_free (ctx);

  if (opt_effects != NULL)
    effect_names = g_strsplit (opt_effects, ",", -1);
  else
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

  src = gst_element_factory_make ("videotestsrc", NULL);
  g_object_set (src, "is-live", TRUE, NULL);

  filter1 = gst_element_factory_make ("capsfilter", NULL);
  gst_util_set_object_arg (G_OBJECT (filter1), "caps", 
      "video/x-raw, width=320, height=240, "
      "format={ I420, YV12, YUY2, UYVY, AYUV, Y41B, Y42B, "
      "YVYU, Y444, v210, v216, NV12, NV21, UYVP, A420, YUV9, YVU9, IYU1 }");

  q1 = gst_element_factory_make ("queue", NULL); 

  blockpad = gst_element_get_static_pad (q1, "src");

  conv_before = gst_element_factory_make ("videoconvert", NULL);

  effect = g_queue_pop_head (&effects);
  cur_effect = effect;

  conv_after = gst_element_factory_make ("videoconvert", NULL);

  q2 = gst_element_factory_make ("queue", NULL);

  filter2 = gst_element_factory_make ("capsfilter", NULL);
  gst_util_set_object_arg (G_OBJECT (filter2), "caps",
      "vidoe/x-raw, width=320, height=240, "
      "format={ RGBx, BGRx, xRGB, xBGR, RGBA, BGRA, ARGB, ABGR, RGB, BGR }");

  sink = gst_element_factory_make ("ximagesink", NULL);

  gst_bin_add_many (GST_BIN (pipeline), src, filter1, q1, conv_before, effect,
      conv_after, q2, sink, NULL);

  gst_element_link_many (src, filter1, q1, conv_before, effect, conv_after, q2, sink, NULL);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  loop = g_main_loop_new (NULL, FALSE);

  gst_bus_add_watch (GST_ELEMENT_BUS (pipeline), bus_cb, loop);

  g_timeout_add_seconds (1, timeout_cb, loop);

  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);

  return 0;
}

/* Note how we added videoconvert elements before and after the effect. This
 * is needed because some elements might operate in different colorspaces; by
 * inserting the conversion elements, we can help ensure a propert format an
 * be negotiated.
 */
