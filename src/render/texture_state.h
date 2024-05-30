#ifndef TEXTURE_STATE_H
#define TEXTURE_STATE_H

#include <stdbool.h>

#include "util/string.h"

typedef struct TextureState {

	// Handle to the texture with which this texture state is associated.
	int textureHandle;

	// Number of animation cycles in the texture with which this texture state is associated.
	unsigned int numAnimations;
	// Index of currently selected animation cycle.
	unsigned int currentAnimation;
	
	// The cell that the current animation starts on.
	unsigned int startCell;
	// Number of frames in currently selected animation cycle.
	unsigned int numFrames;
	// FPS (Frames-per-second) of the currently selected animation cycle.
	unsigned int currentFPS;
	// Current frame in currently selected animation cycle.
	unsigned int currentFrame;
	
	// Time point in milliseconds of last frame update.
	unsigned long long int lastFrameTimeMS;

} TextureState;

TextureState nullTextureState(void);

TextureState newTextureState(const String textureID);

bool textureStateSetAnimation(TextureState *const pTextureState, const unsigned int nextAnimation);

// Returns 0 (false) if an error occurred.
// Returns 1 (true) if function executed successfully with no frame update.
// Returns 2 if function executed successfully with frame update.
int textureStateAnimate(TextureState *const pTextureState);

#endif // TEXTURE_STATE_H
