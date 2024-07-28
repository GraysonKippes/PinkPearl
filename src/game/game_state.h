#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct GameState {
	bool paused;
	bool scrolling;
} GameState;

typedef uint32_t game_state_bitfield_t;

typedef enum game_state_flag_t {
	GAME_STATE_NONE		= 0x00000000,
	GAME_STATE_PAUSED 	= 0x00000001,
	GAME_STATE_SCROLLING	= 0x00000002
} game_state_flag_t;

#endif	// GAME_STATE_H
