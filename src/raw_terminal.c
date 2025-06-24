#define _XOPEN_SOURCE 700  /* for sigaction */

/* C89 standard */
#include <stddef.h>
#include <string.h>

/* POSIX standard */
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "abuf.h"
#include "file.h"

#include "raw_terminal.h"


#define MODE_HEX        0
#define MODE_FORM_CHAR  1
#define MODE_CHAR       2

#define MODE_HEX_INIT        {MODE_HEX, 0, 0, file_append_hexs}
#define MODE_FORM_CHAR_INIT  {MODE_FORM_CHAR, 0, 0, file_append_formatted_chars}
#define MODE_CHAR_INIT       {MODE_CHAR, 0, 0, file_append_chars}

#define STARTING_MODE  &mode_hex

#define PROCESS_KEYPRESS_IGNORE  0
#define PROCESS_KEYPRESS_ACT     0
#define PROCESS_KEYPRESS_QUIT    1
#define PROCESS_KEYPRESS_ERROR   2

#define CTRL_KEY(k) ((k) & 0x1f)

#define VT100_ERASE_LINE  "\x1b[0K"
#define VT100_CUR_TOP_L   "\x1b[1;1H"
#define VT100_CUR_HIDE    "\x1b[?25l"
#define VT100_CUR_SHOW    "\x1b[?25h"


/* -------------------- TYPEDEFS -------------------- */

/* struct containing data about modes */
typedef struct term_mode_tag {
    unsigned char name;
    long int pos;
    unsigned int row_len;
    size_t (*write_func)(abuf_t *, const size_t);
} term_mode_t;


/* -------------------- STATIC VARIABLES -------------------- */

/* defining 3 modes */
static term_mode_t mode_hex = MODE_HEX_INIT;
static term_mode_t mode_form_char = MODE_FORM_CHAR_INIT;
static term_mode_t mode_char = MODE_CHAR_INIT;

/* struct containing signal data (to handle SIGWINCH) */
static struct sig_winch_tag {
    unsigned char error_detected;
    struct sigaction sa;
} sig_winch;

/* struct containing terminal data */
static struct terminal_tag {
    unsigned char is_raw;
    term_mode_t *active_mode;
    unsigned int screen_rows;
    unsigned int screen_cols;
    struct termios initial_state;  /* for preservation of initial state */
} term;


/* -------------------- STATIC PROTOTYPES -------------------- */

/* 
 * Handles SIGWINCH signal, and sets term.screen_rows, term.screen_cols and sigwinch_error_detected accordingly
 * If successful sets sig_winch.error_detected to 0, else 1
 */
static void sigwinch_handler(int sig);

/* Sets row_len of term_mode_t variables based on terminal window size */
static void set_modes_row_len(void);

/* 
 * Uses ioctl() (inside sys/ioctl.h) to get terminal window size
 * If successful returns 0, else 1
 */
static unsigned char get_term_win_size(void);

/* 
 * Changes active mode
 * If successful returns 0, else 1
 */
static unsigned char change_mode(const unsigned char mode);

/* 
 * Handles inputs
 * Returns:
 * - PROCESS_KEYPRESS_IGNORE = no refreshing required
 * - PROCESS_KEYPRESS_ACT = refreshing required
 * - PROCESS_KEYPRESS_QUIT = should exit the program
 * - PROCESS_KEYPRESS_ERROR = error occurred
 */
static unsigned char process_keypress(void);

/* 
 * Reads key from stdin
 * If successful returns 0, else 1
 */
static unsigned char read_key(char *c);

static void refresh_screen(void);

static void draw_rows(abuf_t *ab);


/* -------------------- GLOBAL FUNCTIONS -------------------- */

/* TERMINAL */

unsigned char initialize_term_raw_mode(void) {
    struct termios raw;

    /* Initialize */
    term.is_raw = 0;
    term.active_mode = STARTING_MODE;

    sig_winch.error_detected = 0;

    /* Set signal handler for SIGWINCH, and then raise it to initialize terminal window size */
    memset(&sig_winch.sa, 0, sizeof(sig_winch.sa));
    sig_winch.sa.sa_handler = sigwinch_handler;
    sig_winch.sa.sa_flags = SA_RESTART;
    if (sigaction(SIGWINCH, &sig_winch.sa, NULL) == -1)
        return 1;
    if (raise(SIGWINCH))
        return 2;
    if (sig_winch.error_detected == 1)
        return 3;

    /* Get terminal initial state and save it for later */
    if (tcgetattr(STDIN_FILENO, &term.initial_state) == -1)
        return 4;

    /* Define flags for terminal in raw mode */
    raw = term.initial_state;

    /*
     * BRKINT = turned off so that a break condition will not cause a SIGINT
     * ICRNL = turned off so that Carriage Returns aren't translated into New Lines (fixes CTRL+M)
     * INLCR = turned off so that New Lines aren't tranlsated into Carriage Returns
     * INPCK = turned off to disable parity checking (not on modern terminals)
     * ISTRIP = turned off so the 8th bit is not stripped (this option is probably already off)
     * IXON = turned off to disable software flow control (CTRL+S and CTRL+Q)
     */
    raw.c_iflag &= ~(BRKINT | ICRNL | INLCR | INPCK | ISTRIP | IXON);
    /* OPOST = turned off to disable all output processing features (line '\n' being translated to "\r\n") */
    raw.c_oflag &= ~(OPOST);
    /* CS8 = bit mask to set the character size to 8 bits per byte (probably already set like this) */
    raw.c_cflag |= ~(CS8);
    /*
     * ECHO = turned off to avoid that characters in input are echoed
     * ICANON = turned off to read byte-by-byte insted of line-by-line
     * IEXTEN = turned off so to disable CTRL+V (and CTRL+0 on macOS)
     * ISIG = turned off to avoid sending signals with CTRL+C and CTRL+Z
     */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* VMIN = value that sets the minimum amout of bytes of input needed before theread function can return */
    raw.c_cc[VMIN] = 0;
    /* VMIN = value that sets the maximum amout of time to wait before the read function can return (it's in tenth of a second) */
    raw.c_cc[VTIME] = 1;

    /* Set terminal in the just defined raw mode */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        return 5;

    /* Set terminal in raw mode */
    term.is_raw = 1;

    return 0;
}

unsigned char disable_term_raw_mode(void) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term.initial_state) == -1)
        return 1;
    term.is_raw = 0;
    return 0;
}

unsigned char is_term_raw_mode(void) {
    return term.is_raw;
}

unsigned char term_loop(void) {
    unsigned char flag;

    flag = PROCESS_KEYPRESS_ACT;
    do {
        if (flag == PROCESS_KEYPRESS_ACT)
            refresh_screen();
        flag = process_keypress();
    }
    while (flag == PROCESS_KEYPRESS_ACT || flag == PROCESS_KEYPRESS_IGNORE);

    if (flag == PROCESS_KEYPRESS_QUIT)
        return 0;
    return PROCESS_KEYPRESS_ERROR;
}

/* -------------------- STATIC FUNCTIONS -------------------- */

/* SIGNAL */

static void sigwinch_handler(int sig) {
    switch (sig) {
        case SIGWINCH:
            if (get_term_win_size() == 1) {
                sig_winch.error_detected = 1;
                break;
            }
            sig_winch.error_detected = 0;
            set_modes_row_len();
            break;

        default:
            break;
    }
}

/* TERMINAL */

static void set_modes_row_len(void) {
    mode_hex.row_len = term.screen_cols / 3;
    mode_form_char.row_len = term.screen_cols / 3;
    mode_char.row_len = term.screen_cols;
}

static unsigned char get_term_win_size(void) {
    struct winsize ws;

    /* Tries to use ioctl() with the TIOCGWINSZ request (inside sys/ioctl.h) to get terminal window size */
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_row == 0 || ws.ws_col == 0)
        return 1;

    term.screen_rows = ws.ws_row;
    term.screen_cols = ws.ws_col;
    return 0;
}

static unsigned char change_mode(const unsigned char mode) {
    long int curr_pos;

    /* Get current pos of active mode */
    if ((curr_pos = file_tell()) == -1)
        return 1;

    /* Save current inside active mode (makes it so that MODE_HEX and MODE_FORM_CHAR share pos) */
    switch (term.active_mode->name) {
        case MODE_HEX:
            mode_form_char.pos = curr_pos;
            break;
        case MODE_FORM_CHAR:
            mode_hex.pos = curr_pos;
            break;
    }
    term.active_mode->pos = curr_pos;

    /* Changes active mode */
    switch (mode) {
        case MODE_HEX:
            term.active_mode = &mode_hex;
            break;

        case MODE_CHAR:
            term.active_mode = &mode_char;
            break;

        case MODE_FORM_CHAR:
            term.active_mode = &mode_form_char;
            break;
        
        default:
            return 1;
    }
    if (file_seek_set(term.active_mode->pos) == 1)
        return 1;
    return 0;
}

/* INPUT */

static unsigned char process_keypress(void) {
    unsigned int i;
    long int row_len;
    char c;

    row_len = (long int)term.active_mode->row_len;

    read_key(&c);
    switch (c) {
        case CTRL_KEY('q'):
            return PROCESS_KEYPRESS_QUIT;

        case 'w':
        case 'W':
            if (file_move(-1 * row_len) == 1)
                return 2;
            return PROCESS_KEYPRESS_ACT;

        case 's':
        case 'S':
            if (file_will_be_end(row_len) == 0) {
                if (file_move(row_len) == 1)
                    return PROCESS_KEYPRESS_ERROR;
                return PROCESS_KEYPRESS_ACT;
            } else if (file_will_be_end(row_len) == 1)
                return PROCESS_KEYPRESS_ACT;
            else
                return PROCESS_KEYPRESS_ERROR;

        case 'a':
        case 'A':
            if (file_move(-1 * (long int)term.screen_rows * row_len) == 1)
                return PROCESS_KEYPRESS_ERROR;
            return PROCESS_KEYPRESS_ACT;

        case 'd':
        case 'D':
            for (i = 0; i < term.screen_rows; i++) {
                if (file_will_be_end(row_len) == 0) {
                    if (file_move(row_len) == 1)
                        return PROCESS_KEYPRESS_ERROR;
                } else if (file_will_be_end(row_len) == 1)
                    return PROCESS_KEYPRESS_ACT;
                else
                    return PROCESS_KEYPRESS_ERROR;
            }
            return PROCESS_KEYPRESS_ACT;

        case 'h':
        case 'H':
            if (term.active_mode->name == MODE_HEX)
                return PROCESS_KEYPRESS_IGNORE;
            if (change_mode(MODE_HEX) == 1)
                return PROCESS_KEYPRESS_ERROR;
            return PROCESS_KEYPRESS_ACT;

        case 'c':
        case 'C':
            if (term.active_mode->name == MODE_FORM_CHAR)
                return PROCESS_KEYPRESS_IGNORE;
            if (change_mode(MODE_FORM_CHAR) == 1)
                return PROCESS_KEYPRESS_ERROR;
            return PROCESS_KEYPRESS_ACT;

        case CTRL_KEY('c'):
            if (term.active_mode->name == MODE_CHAR)
                return PROCESS_KEYPRESS_IGNORE;
            if (change_mode(MODE_CHAR) == 1)
                return PROCESS_KEYPRESS_ERROR;
            return PROCESS_KEYPRESS_ACT;
        
        default:
            return PROCESS_KEYPRESS_IGNORE;
    }
}

static unsigned char read_key(char *c) {
    ssize_t nread;
    while ((nread = read(STDIN_FILENO, c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN)
            return 1;
    }
    return 0;
}

/* OUTPUT */

static void refresh_screen(void) {
    abuf_t ab = ABUF_INIT;

    ab_append(&ab, VT100_CUR_HIDE, sizeof(VT100_CUR_HIDE) - 1);
    ab_append(&ab, VT100_CUR_TOP_L, sizeof(VT100_CUR_TOP_L) - 1);
    draw_rows(&ab);
    ab_append(&ab, VT100_CUR_TOP_L, sizeof(VT100_CUR_TOP_L) - 1);
    ab_append(&ab, VT100_CUR_SHOW, sizeof(VT100_CUR_SHOW) - 1);

    write(STDOUT_FILENO, ab.b, ab.len);

    ab_free(&ab);
}

static void draw_rows(abuf_t *ab) {
    size_t bytes;
    unsigned int y;

    bytes = 0;
    for (y = 0; y < term.screen_rows; y++) {

        bytes += term.active_mode->write_func(ab, term.active_mode->row_len);

        ab_append(ab, VT100_ERASE_LINE, sizeof(VT100_ERASE_LINE) - 1);
        if (y < term.screen_rows - 1)
            ab_append(ab, "\r\n", 2);
    }

    file_move(-1 * ((long int)bytes));  /* DANGEROUS: size_t to long int */
}
