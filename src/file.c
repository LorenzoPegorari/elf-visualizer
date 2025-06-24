/* C89 standard */
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abuf.h"

#include "file.h"


/*#define BUFFER_SIZE 256*/

#define FILE_TAG_INIT   {0, 0, NULL}

/*
static const char STR000[] = "ELF magic number: 0x7F454c46 (0x7F E L F)\n";
static const char *STR001[] = {"Format: 32-bit\n", "Format: 64-bit\n"};
static const char *STR002[] = {"Endianness: little (least significant byte of word at smallest memory address)\n",
                               "Endianness: big (most significant byte of word at smallest memory address)\n"};
static const char STR003[] = "ELF version: 1 (only version)\n";
static const char *STR004[] = {"OS ABI: System V\n",
                               "OS ABI: HU-UX\n",
                               "OS ABI: NetBSD\n",
                               "OS ABI: Linux\n",
                               "OS ABI: GNU Hard\n",
                               "OS ABI: Solaris\n",
                               "OS ABI: AIX (Monterey)\n",
                               "OS ABI: IRIX\n",
                               "OS ABI: FreeBSD\n",
                               "OS ABI: Tru64\n",
                               "OS ABI: Novell Modesto\n",
                               "OS ABI: OpenBSD\n",
                               "OS ABI: OpenVMS\n",
                               "OS ABI: NonStop Kernel\n",
                               "OS ABI: AROS\n",
                               "OS ABI: FenixOS\n",
                               "OS ABI: Nuxi CloudABI\n",
                               "OS ABI: Stratus Technologies OpenVOS\n"};
static const char STR005[] = "?\n";
static const char STR006[] = "Reserved padding bytes (not currently used)\n";
*/

/* -------------------- STATIC VARIABLES -------------------- */

/* struct for file data */
static struct file_tag {
    long int len;
    unsigned char is_open;
    FILE *h;
} file = FILE_TAG_INIT;


/* -------------------- GLOBAL FUNCTIONS -------------------- */

/* OPEN / CLOSE / GETTERS */

unsigned char file_open(const char *__filename, const char *__modes) {
    file.h = fopen(__filename, __modes);
    if (file.h == NULL) {
        return 1;
    }
    file.is_open = 1;

    if (fseek(file.h, 0, SEEK_END) == -1)
        return 1;
    if ((file.len = file_tell()) == -1)
        return 1;
    if (fseek(file.h, 0, SEEK_SET) == -1)
        return 1;

    return 0;
}

unsigned char file_close(void) {
    if (fclose(file.h) == EOF)
        return 1;
    file.is_open = 0;
    return 0;
}

unsigned char is_file_open(void) {
    return file.is_open;
}

/* READ */

size_t file_append_bytes(abuf_t *ab, const size_t len) {
    size_t n_bytes_read;
    char *temp;

    if ((temp = malloc(len)) == NULL)
        return 0;
    
    if ((n_bytes_read = fread(temp, 1, len, file.h)) < len && !feof(file.h)) {
        free(temp);
        return 0;
    }

    if (ab_append(ab, temp, n_bytes_read)) {
        free(temp);
        return 0;
    }
    
    free(temp);

    return n_bytes_read;
}

size_t file_append_hexs(abuf_t *ab, const size_t len) {
    unsigned int i;
    size_t n_chars_read;
    abuf_t temp = ABUF_INIT;
    char *temp_long;

    n_chars_read = file_append_bytes(&temp, len);
    if (n_chars_read == 0) {
        ab_free(&temp);
        return 0;
    }

    if ((temp_long = malloc(n_chars_read * 3)) == NULL)
        return 0;

    for (i = 0; i < n_chars_read; i++) {
        sprintf(&temp_long[i * 3], "%02X", temp.b[i]);
        if (i < n_chars_read - 1)
            temp_long[i * 3 + 2] = ' ';
    }

    ab_free(&temp);

    if (ab_append(ab, temp_long, n_chars_read * 3 - 1)) {
        free(temp_long);
        return 0;
    }

    free(temp_long);
    
    return n_chars_read;
}

size_t file_append_formatted_chars(abuf_t *ab, const size_t len) {
    unsigned int i;
    size_t n_chars_read;
    abuf_t temp = ABUF_INIT;
    char *temp_long;

    n_chars_read = file_append_bytes(&temp, len);
    if (n_chars_read == 0) {
        ab_free(&temp);
        return 0;
    }

    if ((temp_long = malloc(n_chars_read * 3 - 1)) == NULL)
        return 0;

    for (i = 0; i < n_chars_read; i++) {
        temp_long[i * 3] = ' ';
        if (isprint(temp.b[i]) == 0)
            temp_long[i * 3 + 1] = '.';
        else
            temp_long[i * 3 + 1] = temp.b[i];
        if (i < n_chars_read - 1)
            temp_long[i * 3 + 2] = ' ';
    }

    ab_free(&temp);

    if (ab_append(ab, temp_long, n_chars_read * 3 - 1)) {
        free(temp_long);
        return 0;
    }

    free(temp_long);
    
    return n_chars_read;
}

size_t file_append_chars(abuf_t *ab, const size_t len) {
    unsigned int i;
    size_t n_chars_read;
    abuf_t temp = ABUF_INIT;

    n_chars_read = file_append_bytes(&temp, len);
    if (n_chars_read == 0) {
        ab_free(&temp);
        return 0;
    }

    for (i = 0; i < n_chars_read; i++) {
        if (isprint(temp.b[i]) == 0)
            temp.b[i] = '.';
    }

    if (ab_append(ab, temp.b, n_chars_read)) {
        ab_free(&temp);
        return 0;
    }

    ab_free(&temp);
    return n_chars_read;
}

/* MOVE */

unsigned char file_move(const long int bytes) {
    if (fseek(file.h, bytes, SEEK_CUR) == -1) {
        if (fseek(file.h, 0, SEEK_SET) == -1)
            return 1;
        return 0;
    }
    return 0;
}

unsigned char file_will_be_end(const long int bytes) {
    long int pos;
    unsigned char will_be_end;

    if (fseek(file.h, bytes, SEEK_CUR) == -1)
        return 2;

    if ((pos = file_tell()) == -1)
        return 2;
    if (pos < file.len)
        will_be_end = 0;
    else
        will_be_end = 1;
    
    if (fseek(file.h, -1 * bytes, SEEK_CUR) == -1)
        return 2;

    return will_be_end;
}

long int file_tell(void) {
    long int pos;
    if ((pos = ftell(file.h)) < 0)
        return -1;
    return pos;
}

unsigned char file_seek_set(const long bytes) {
    if (fseek(file.h, bytes, SEEK_SET) == -1)
        return 1;
    return 0;
}

/* TAbLE */

/*unsigned char generate_elf_table(const char *__filename, const char *__modes) {
    unsigned char buf[256];
    
    elf_table.h = fopen(__filename, __modes);
    if (elf_table.h == NULL)
        return 1;

    if (fread(buf, 1, 4, file.h) != 4)
        return 1;
    if ((memcmp(buf, "\x7f\x45\x4c\46", 4)))
        return 1;
    if (ab_append(&elf_table.text, STR000, sizeof(STR000) - 1))
        return 1;
    
    if (fread(buf, 1, 1, file.h) != 1)
        return 1;
    if (buf[0] < 2)
        return 1;
    if (ab_append(&elf_table.text, STR001[buf[0]], sizeof(STR001[buf[0]]) - 1))
        return 1;

    if (fread(buf, 1, 1, file.h) != 1)
        return 1;
    if (buf[0] < 2)
        return 1;
    if (ab_append(&elf_table.text, STR002[buf[0]], sizeof(STR002[buf[0]]) - 1))
        return 1;

    if (fread(buf, 1, 4, file.h) != 4)
        return 1;
    if (buf[0] != '\x01')
        return 1;
    if (ab_append(&elf_table.text, STR003, sizeof(STR003) - 1))
        return 1;
    
    if (fread(buf, 1, 1, file.h) != 1)
        return 1;
    if (buf[0] < 13)
        return 1;
    if (ab_append(&elf_table.text, STR004[buf[0]], sizeof(STR004[buf[0]]) - 1))
        return 1;
    
    if (fread(buf, 1, 1, file.h) != 1)
        return 1;
    if (ab_append(&elf_table.text, STR005, sizeof(STR005) - 1))
        return 1;

    if (fread(buf, 1, 7, file.h) != 7)
        return 1;
    if ((memcmp(buf, "\x00\x00\x00\x00\x00\x00\x00", 7)))
        return 1;
    if (ab_append(&elf_table.text, STR006, sizeof(STR006) - 1))
        return 1;
    return 0;
}*/
