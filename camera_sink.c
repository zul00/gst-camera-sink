#include <gst/gst.h>
#include <glib.h>

const char* dev="/dev/video1";
const char* file="this.yuv";

typedef struct {
  GstElement *pipeline;
  GstElement *source, *sink, *caps, *dec;

  guint bus_watch_id;

  GMainLoop *loop;  /* GLib's Main Loop */
} GstData;

/* The appsink has received a buffer */                                         
static void new_sample (GstElement *sink, GstData *data) {                   
  g_printerr ("In the callback function.\n");                                   
                                                                                
  GstSample *sample;                                                            
  /* Retrieve the buffer */                                                     
  g_signal_emit_by_name (sink, "pull-sample", &sample);                         
  if (sample) {                                                                 
    /* The only thing we do in this example is print a * to indicate a received buffer */
    g_print ("*");                                                              
    gst_sample_unref (sample);                                                  
  }                                                                             
}

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
  GstData data;
  GstBus *bus;

  GstMessage *msg;
  GstCaps *filtercaps;

  /* Initialization */
  gst_init(&argc, &argv);
  data.loop = g_main_loop_new(NULL, FALSE);

  /* Create the elements */
  data.pipeline  = gst_pipeline_new("pipeline");
  data.source    = gst_element_factory_make("v4l2src",     "source");
  data.caps      = gst_element_factory_make("capsfilter",  "caps");
  data.dec       = gst_element_factory_make("jpegdec",     "dec");
  data.sink      = gst_element_factory_make("appsink",     "sink");

  if (!data.pipeline || !data.source || !data.sink || !data.caps || !data.dec) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }
  
  /* Add message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (data.pipeline));
  data.bus_watch_id = gst_bus_add_watch (bus, bus_call, data.loop);
  gst_object_unref (bus);

  /* Build the pipeline */
  gst_bin_add_many (GST_BIN (data.pipeline), data.source, data.caps, data.sink, data.dec, NULL);

  /* Setup pipeline */
  // Source properties
  g_object_set (data.source, "device", dev, NULL);
  g_printerr ("Source configured.\n");
  
  // Filter properties
  filtercaps = gst_caps_new_simple ("image/jpeg",
      "width", G_TYPE_INT, 160,
      "height", G_TYPE_INT, 120,
      "framerate", GST_TYPE_FRACTION, 30, 1,
      NULL);
  g_object_set (data.caps, "caps", filtercaps, NULL);
  gst_caps_unref (filtercaps);
  g_printerr ("Filter configured.\n");

  // Sink properties
  g_object_set (data.sink, "emit-signals", TRUE, "drop", TRUE, NULL);
  g_signal_connect (data.sink, "new-sample", G_CALLBACK (new_sample), &data);
  gst_caps_unref (Appsink);
  g_printerr ("Appsink configured.\n");

  /* Link pipeline */
  if (gst_element_link_many (data.source, data.caps, data.dec, data.sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }

  /* Start playing */
  gst_element_set_state (data.pipeline, GST_STATE_PLAYING);

  g_print ("Running...");
  g_main_loop_run (data.loop);

  /* Out of the main loop, clean up nicely */
  g_print ("Returned\n");
  gst_element_set_state (data.pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (data.pipeline));
  g_source_remove (data.bus_watch_id);
  g_main_loop_unref (data.loop);

  return 0;
}
