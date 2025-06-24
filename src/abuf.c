/* C89 standard */
#include <stdlib.h>
#include <string.h>

#include "abuf.h"


unsigned int ab_append(abuf_t *ab, const char *s, const size_t len) {
    char *new_b;

    if ((new_b = realloc(ab->b, ab->len + len)) == NULL)
        return 1;

    memcpy(&new_b[ab->len], s, len);
    ab->b = new_b;
    ab->len += len;
    return 0;
}

void ab_free(abuf_t *ab) {
    free(ab->b);
}
