/*
gcc -o serialization3 serialization3.c `pkg-config --libs --cflags gstreamer-1.0`
*/

#include <stdlib.h>
#include <stdio.h>
#include <gst/gst.h>
#include <assert.h>

/** Turn a string into a GstStructure. Then, serialize that structure, and
 * assert the two strings match.
 *
 * The serialization is done with gst_structure_get_value, which is the second
 * most general gst_structure_get* method to use.
 */
int main(int argc, char* argv[argc]) {
    gst_init(&argc, &argv);
    GstStructure* it = gst_structure_new_from_string("it,what=wow");
    // Assert the strings match
    GValue const*const what = gst_structure_get_value(it, "what");
    assert(!strcmp(g_value_get_string(what), "wow"));
    g_free(it);
    puts("\nPASS");
    return EXIT_SUCCESS;
}
