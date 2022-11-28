/*
clear && gcc basic-tutorial-4.c -o basic-tutorial-4 `pkg-config --cflags --libs gstreamer-1.0` && ./basic-tutorial-4
*/

/* DESCRIPTION
 * When you run the line above, you should see a window appear with the Sintel
 * trailer video. You should hear the audio of the trailer video. In the
 * terminal, you will see the stream position being updated to the nanosecond
 * continuously. At the 10 second mark, the position will seek to near the
 * end of the video. That's basically the end of the demo.
 */

/* CONCEPTS
 * - Query the pipeline for information with GstQuery
 * - Obtain common information like position and duration using
 *   gst_element_query_position() and gst_element_query_duration()
 * - Seek to an arbitrary position in the stream using gst_element_seek_simple()
 * - Know which states allow these operations
 */

#include <gst/gst.h>

/* Structure to contain all our nformation, so we can pass it around */
typedef struct _CustomData {
  GstElement *playbin; /* Our one and only element */
  gboolean playing; /* Are we in the PLAYING state? */
  gboolean terminate; /* Should we terminate execution? */
  gboolean seek_enabled; /* Is seeking enabled for this media? */
  gboolean seek_done; /* Have we performed the seek already? */
  gint64 duration; /* How long dose this media last, in nanoseconds? */
} CustomData;

/* Forward declaration of the message processing function */
static void
handle_message (CustomData *data, GstMessage* msg);

int
main (int argc, char *argv[])
{
  CustomData data;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;

  data.playing = FALSE;
  data.terminate = FALSE;
  data.seek_enabled = FALSE;
  data.seek_done = FALSE;
  data.duration = FALSE;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Create the elements */
  if ((data.playbin = gst_element_factory_make ("playbin", "playbin")) == NULL)
    {
      g_printerr ("Not all elements culd e created.\n");
      return EXIT_FAILURE;
    }

  /* Set the URI to play */
  g_object_set (data.playbin, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);

  /* Start playing */
  ret = gst_element_set_state (data.playbin, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE)
    {
      g_printerr ("Unable to set the pipeline to the playing state.\n");
      gst_object_unref (data.playbin);
      return EXIT_FAILURE;
    }

  /* Listen to the bus */
  bus = gst_element_get_bus (data.playbin);
  do
    {
      // Get a message
      msg = gst_bus_timed_pop_filtered (bus, 100 * GST_MSECOND,
          GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR |
          GST_MESSAGE_EOS | GST_MESSAGE_DURATION);

      /* Parse the message */
      if (msg != NULL)
        {
          /* Handle the message. This function is just to keep main() from
           * being too bloated.
           */
          handle_message (&data, msg);
        }
      else
        {
          /* We didn't get a message, which means the timeout expired. */
          if (data.playing)
            {
              gint64 current = -1;

              /* Query the current position of the stream */
              if (!gst_element_query_position (data.playbin,
                                               GST_FORMAT_TIME,
                                               &current))
                {
                  g_printerr ("Could not query current position.\n");
                }

              /* If we didn't know it yet, query the stream duration */
              if (!GST_CLOCK_TIME_IS_VALID (data.duration))
                {
                  if (!gst_element_query_duration (data.playbin,
                                                   GST_FORMAT_TIME,
                                                   &data.duration))
                    {
                      g_printerr ("Could not query current duration.\n");
                    }
                }

              /* Print current position and total duration */
              g_print ("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
                  GST_TIME_ARGS (current), GST_TIME_ARGS (data.duration));

              /* If seeking is enabled, we have not done it yet, and the time
               * is right, then seek. I think the concern there on the last part
               * is that before 10s not enough of the stream will be available
               * to seek properly.
               */
              if (data.seek_enabled && !data.seek_done && current > 10 * GST_SECOND)
                {
                  g_print ("\nReached 10s, performing seek...\n");
                  gst_element_seek_simple (data.playbin, GST_FORMAT_TIME,
                      GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
                      30 * GST_SECOND);
                  data.seek_done = TRUE;
                }
            }
        }
    }
  while (!data.terminate); 

  /* Clean up */
  gst_object_unref (bus);
  gst_element_set_state (data.playbin, GST_STATE_NULL);
  gst_object_unref (data.playbin);

  return EXIT_SUCCESS;
}

static void
handle_message (CustomData *data, GstMessage *msg)
{
  GError *err;
  gchar *debug_info;

  switch (GST_MESSAGE_TYPE (msg))
    {
      case GST_MESSAGE_ERROR:
        gst_message_parse_error (msg, &err, &debug_info);
        g_printerr ("Error received from elment %s: %s\n",
            GST_OBJECT_NAME (msg->src), err->message);
        g_printerr ("Debugging information %s\n",
            debug_info ? debug_info : "none");
        g_clear_error (&err);
        g_free (debug_info);
        data->terminate = TRUE;
        break;

      case GST_MESSAGE_EOS:
        g_print ("\nEnd-Of-Stream reached.\n");
        data->terminate = TRUE;
        break;

      case GST_MESSAGE_DURATION:
        data->duration = GST_CLOCK_TIME_NONE;
        break;

      case GST_MESSAGE_STATE_CHANGED: {
        GstState old_state;
        GstState new_state;
        GstState pending_state;
        gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
        if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->playbin))
          {
            g_print ("Pipeline state changed from %s to %s\n",
                     gst_element_state_get_name (old_state),
                     gst_element_state_get_name (new_state));

            /* Remember whether we are in the PLAYING state or not */
            data->playing = new_state == GST_STATE_PLAYING;

            if (data->playing)
              {
                /* We just moved to PLAYING. Check if seeking is possible */
                GstQuery *query;
                gint64 start, end;
                query = gst_query_new_seeking (GST_FORMAT_TIME);
                if (gst_element_query (data->playbin, query))
                  {
                    gst_query_parse_seeking (query, NULL, &data->seek_enabled,
                                             &start, &end);

                    if (data->seek_enabled)
                      {
                        g_print ("Seeking is ENABLED from %" GST_TIME_FORMAT
                                 " to %" GST_TIME_FORMAT
                                 "\n", GST_TIME_ARGS (start),
                                 GST_TIME_ARGS (end));
                      }
                    else
                      {
                        g_print ("Seeking is DISABLED for this stream.\n");
                      }
                  }
                else
                  {
                    g_printerr ("Seeking query failed.");
                  }
                gst_query_unref (query);
              }
          }
        break;
      }

      default:
        /* Should not reach here */
        g_printerr ("Unexpected message received.\n");
        break;
    }
    gst_message_unref (msg);
}
