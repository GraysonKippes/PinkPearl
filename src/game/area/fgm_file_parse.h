#ifndef FGM_FILE_PARSE_H
#define FGM_FILE_PARSE_H

// This module contains a set of functions that parse
// .fgm (Fox Game Master) data files.

#include "area.h"
#include "room.h"

Area readAreaData(const char *const pFilename);

#endif	// FMG_FILE_PARSE
