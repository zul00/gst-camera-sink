#include <gst/gst.h>
#include <glib.h>

/**
 * @brief Message handler
 */
static gboolean bus_call (
    GstBus     *bus,
    GstMessage *msg,
    gpointer    data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

int main(int argc, char *argv[]) 
{
  GMainLoop *loop;
  GstBus *bus;
  guint bus_watch_id;

  GstElement *pipeline, *source, *sink, *caps, *dec;
  GstMessage *msg;
  GstStateChangeReturn ret;
  GstCaps *filtercaps;

  /* Initialization */
  gst_init(&argc, &argv);
  loop = g_main_loop_new(NULL, FALSE);

  /* Create the elements */
  pipeline  = gst_pipeline_new("test-pipeline");
  source    = gst_element_factory_make("v4l2src",     "source");
  caps      = gst_element_factory_make("capsfilter",  "caps");
  dec       = gst_element_factory_make("jpegdec",     "dec");
  sink      = gst_element_factory_make("filesink",    "sink");

  if (!pipeline || !source || !sink || !caps || !dec) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }
  
  /* Add message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  /* Build the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, caps, sink, dec, NULL);
  if (gst_element_link_many (source, caps, dec, sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Setup pipeline */
  // Source properties
  g_object_set (source, "device", "/dev/video0", NULL);
  
  // Filter properties
  filtercaps = gst_caps_new_simple ("image/jpeg",
      "width", G_TYPE_INT, 160,
      "height", G_TYPE_INT, 120,
      "framerate", GST_TYPE_FRACTION, 30, 1,
      NULL);
  g_object_set (caps, "caps", filtercaps, NULL);
  gst_caps_unref (filtercaps);

  // Sink properties
  g_object_set(sink, "location", "file2.yuv", NULL);

  /* Start playing */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  g_print ("Running...");
  g_main_loop_run (loop);

  /* Out of the main loop, clean up nicely */
  g_print ("Returned\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}
