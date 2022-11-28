/*
gcc -o serialization4 serialization4.c `pkg-config --libs --cflags gstreamer-1.0`
*/

#include <stdlib.h>
#include <stdio.h>
#include <gst/gst.h>
#include <assert.h>

/** Turn a string into a GstStructure. Then, serialize that structure, and
 * assert the two strings match.
 *
 * The serialization is done with gst_structure_get_string, which is the
 * least general gst_structure_get* method to use to get a string.
 */
int main(int argc, char* argv[argc]) {
    gst_init(&argc, &argv);
    GstStructure* it = gst_structure_new_from_string("it,what=wow");
    // Assert the strings match
    gchar const*const what = gst_structure_get_string(it, "what");
    assert(!strcmp(what, "wow"));
    g_free(it);
    puts("\nPASS");
    return EXIT_SUCCESS;
}
