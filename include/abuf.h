#ifndef _ABUF_H_
#define _ABUF_H_


/* C89 standard */
#include <stddef.h>


#define ABUF_INIT  {NULL, 0}


/* struct for string that supports append method */
typedef struct abuf_tag {
    char *b;
    size_t len;
} abuf_t;


/* 
 * Appends len bytes from string s to ab
 * If successful returns 0, else 1 
 */
unsigned int ab_append(abuf_t *ab, const char *s, const size_t len);

/* Frees ab */
void ab_free(abuf_t *ab);


#endif
