#ifndef DEBUG_H
#define DEBUG_H

#include <stdbool.h>

#define DEBUG

#ifdef DEBUG
const bool debug_enabled = true;
#elif	// DEBUG
const bool debug_enabled = false;
#endif	// DEBUG

#endif	// DEBUG_H
