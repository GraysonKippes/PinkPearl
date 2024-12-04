#ifndef GAME_H
#define GAME_H

#include "area/area.h"

extern Area currentArea;

void start_game(void);

void tick_game(void);

bool isGamePaused(void);

#endif	// GAME_H
