#ifndef FGM_FILE_PARSE_H
#define FGM_FILE_PARSE_H

// This module contains a set of functions that parse
// .fgm (Fox Game Master) data files.

#include "area.h"
#include "room.h"

area_t parse_fga_file(const char *filename);

#endif	// FMG_FILE_PARSE
