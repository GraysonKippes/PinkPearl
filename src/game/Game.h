#ifndef GAME_H
#define GAME_H

#include "area/area.h"

// Stores information about the Pink Pearl game, such as whether or not the game is paused.
typedef struct GameState {
	bool paused;
	bool scrolling;
} GameState;

extern Area currentArea;

void start_game(void);

void endGame(void);

void tick_game(void);

GameState getGameState(void);

#endif	// GAME_H
