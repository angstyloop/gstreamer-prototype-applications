/*
gcc -o serialization1 serialization1.c `pkg-config --libs --cflags gstreamer-1.0`
*/

#include <stdlib.h>
#include <stdio.h>
#include <gst/gst.h>
#include <assert.h>

/** Turn a string into a GstStructure. Then, serialize that structure, and
 * assert the two strings match.
 *
 * The serialization is done with gst_structure_get, which is the most general
 * gst_structure_get* method to use.
 */
int main(int argc, char* argv[argc]) {
    gst_init(&argc, &argv);
    GstStructure* it = gst_structure_new_from_string("it,what=wow");
    gchar* what = NULL;
    gst_structure_get(it, "what", G_TYPE_STRING, &what, NULL);
    // Assert the strings match
    assert(!strcmp(what, "wow"));
    g_free(what);
    g_free(it);
    puts("PASS");
    return EXIT_SUCCESS;
}
