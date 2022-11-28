/*
clear && gcc -o serialization5 serialization5.c `pkg-config --libs --cflags gstreamer-1.0` && ./serialization5
*/

#include <stdlib.h>
#include <stdio.h>
#include <gst/gst.h>
#include <assert.h>

/** Create a GstStructure. Then print its contents.
 */
int main(int argc, char* argv[argc]) {
    gst_init(&argc, &argv);
    // Field names should be passed as name, type, value.
    int i = 1;
    GstStructure* it = gst_structure_new("it",
                                         "what", G_TYPE_STRING, "wow",
                                         "int", G_TYPE_INT, i,
                                         NULL);
    gchar const*const what = gst_structure_get_string(it, "what");
    int i2 = 0;
    if (!gst_structure_get_int(it, "int", &i2)) {
      g_print("gst_structure_get_int(it, \"int\") ERROR");
      return EXIT_FAILURE;
    }
    // Assert the strings match
    assert(!strcmp(what, "wow"));
    // Assert the ints match
    assert(i == i2);
    g_free(it);
    puts("\nPASS");
    return EXIT_SUCCESS;
}
