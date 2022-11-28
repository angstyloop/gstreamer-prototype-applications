/*
gcc -o serialization2 serialization2.c `pkg-config --libs --cflags gstreamer-1.0`
*/

#include <stdlib.h>
#include <stdio.h>
#include <gst/gst.h>
#include <assert.h>

/** Create an empty GstStructure. Then, get its name, and assert it's equal to
 * what you named it.
 */
int main(int argc, char* argv[argc]) {
    gst_init(&argc, &argv);
    GstStructure* it = gst_structure_new_empty("it");
    assert(!strcmp(gst_structure_get_name(it), "it"));
    g_free(it);
    puts("PASS");
    return EXIT_SUCCESS;
}
