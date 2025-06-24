#ifndef _RAW_TERMINAL_H_
#define _RAW_TERMINAL_H_


/* 
 * Initialize terminal data, assigns SIGWINCH signal handler and enables raw mode
 * If successful returns 0, else:
 * - 1 = couldn't set sigaction for SIGWINCH
 * - 2 = couldn't raise SIGWINCH
 * - 3 = couldn't get terminal size
 * - 4 = couldn't get terminal initial state
 * - 5 = couldn't set terminal raw state
 */
unsigned char initialize_term_raw_mode(void);

/* 
 * Disables raw mode for terminal, returning to initial state
 * If successful returns 0, else 1 
 */
unsigned char disable_term_raw_mode(void);

/* If terminal is in raw mode returns 1, else 0 */
unsigned char is_term_raw_mode(void);

/* 
 * Enters in terminal loop
 * If "quit input" was received returns 0, else if error returns 1 
 */
unsigned char term_loop(void);


#endif
