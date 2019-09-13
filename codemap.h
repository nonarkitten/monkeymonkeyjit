/*

Simple search for the list of list of compiled chunks of code

--------------------------------------------------------------------------------

We'll use a simple hash map for the search with the items within each bucket
sorted by last-recently-used (e.g., new items are inserted at the head, and not
the tail). 

Monkeymonkeyjit always compiles code into blocks -- there is no 'interpreter
mode'. As such, code that is only used once will clutter up the interpreter and
slow down performance. As such, some sort of garbage collection based on usage
is necessary.

*/

#include "main.h"

#ifndef __CODEMAP_H__
#define __CODEMAP_H__

// Returns NULL if address has no code associated with it
// Performs no check on whether address is a valid M68K entry point
uint16_t* get_code_ptr(uint32_t address);

// Sets the code pointer for the specified address, creating a new
// node if one is not there presently, and will free and overwrite
// the one that is.
void set_code_ptr(uint32_t address, uint16_t* code);

// Before exiting, we should ensure all pointers are freed
void free_code_ptrs(void);

#endif /* __CODEMAP_H__ */
