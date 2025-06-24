/* C89 standard */
#include <stdio.h>
#include <stdlib.h>

#include "file.h"
#include "raw_terminal.h"


#define ERROR001  "ERROR: Argument missing!\n"
#define ERROR002  "ERROR: Could not open file!\n"
#define ERROR003  "ERROR: Could not close opened file!\n"
#define ERROR004  "ERROR: Could not set sigaction for SIGWINCH!\n"
#define ERROR005  "ERROR: Could not raise SIGWINCH!\n"
#define ERROR006  "ERROR: Could not get terminal size!\n"
#define ERROR007  "ERROR: Could not get terminal initial state!\n"
#define ERROR008  "ERROR: Could not set terminal raw state!\n"


/* -------------------- STATIC PROTOTYPES -------------------- */

/* Handles exit() (used in atexit()) */
static void handle_exit(void);


/* -------------------- MAIN -------------------- */

int main(int argc, char *argv[]) {
    /* Declarations */
    unsigned char status;

    /* Arguments */
    if (argc < 2) {
        fprintf(stderr, ERROR001);
        exit(EXIT_FAILURE);
    }

    /* Open file */
    if (file_open(argv[1], "rb")) {
        fprintf(stderr, ERROR002);
        exit(EXIT_FAILURE);
    }

    /* Set terminal in raw mode */
    status = initialize_term_raw_mode();
    if (status > 0) {
        switch (status) {
            case 1:
                fprintf(stderr, ERROR004);
                break;

            case 2:
                fprintf(stderr, ERROR005);
                break;

            case 3:
                fprintf(stderr, ERROR006);
                break;

            case 4:
                fprintf(stderr, ERROR007);
                break;

            case 5:
                fprintf(stderr, ERROR008);
                break;
        }
        exit(EXIT_FAILURE);
    }

    printf("test\r\n");
    /* Initialize exit_handler function */
    atexit(handle_exit);

    /* Main loop */
    status = term_loop();

    if (status == 1)
        exit(EXIT_FAILURE);
    exit(EXIT_SUCCESS);
}


/* -------------------- STATIC FUNCTIONS -------------------- */

/* EXIT */
static void handle_exit(void) {
    /* Handles terminal if in raw mode */
    if (is_term_raw_mode() == 1) {
        if (disable_term_raw_mode() == 1)
            fprintf(stderr, ERROR008);
    }

    /* Handles file if open */
    if (is_file_open() == 1) {
        if (file_close() == 1)
            fprintf(stderr, ERROR003);
    }
}
