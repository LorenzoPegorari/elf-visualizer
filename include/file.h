#ifndef _FILE_H_
#define _FILE_H_


/* C89 standard */
#include <stddef.h>

#include "abuf.h"


/* 
 * Opens specified file with specified mode and sets is_file_open
 * If successful returns 0, else 1
 */
unsigned char file_open(const char *__filename, const char *__modes);

/* 
 * Closes opened file
 * If successful returns 0, else 1
 */
unsigned char file_close(void);

/* If file is open returns 1, else 0 */
unsigned char is_file_open(void);

/* 
 * Closes opened file
 * If successful returns 0, else 1
 */
size_t file_append_bytes(abuf_t *ab, const size_t len);

size_t file_append_hexs(abuf_t *ab, const size_t len);

size_t file_append_formatted_chars(abuf_t *ab, const size_t len);

size_t file_append_chars(abuf_t *ab, const size_t len);

unsigned char file_move(const long int bytes);

unsigned char file_will_be_end(const long int bytes);

long int file_tell(void);

unsigned char file_seek_set(const long bytes);

/*unsigned char generate_elf_table(const char *__filename, const char *__modes);*/

#endif
