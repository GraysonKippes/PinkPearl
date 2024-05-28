#include "texture_state.h"

#include <stddef.h>

#include "log/logging.h"
#include "util/allocate.h"
#include "util/time.h"
#include "vulkan/texture.h"
#include "vulkan/texture_manager.h"

TextureState nullTextureState(void) {
	return (TextureState){
		.textureHandle = missing_texture_handle,
		.numAnimations = 0,
		.currentAnimation = 0,
		.numFrames = 0,
		.currentFPS = 0,
		.currentFrame = 0,
		.lastFrameTimeMS = 0
	};
}

TextureState newTextureState(const String textureID) {
	
	TextureState textureState = nullTextureState();
	
	if (is_string_null(textureID)) {
		log_message(ERROR, "Error finding loaded texture: given texture ID is NULL.");
		return textureState;
	}
	
	textureState.textureHandle = findTexture(textureID);
	if (!validateTextureHandle(textureState.textureHandle)) {
		textureState.textureHandle = missing_texture_handle;
		return textureState;
	}
	
	Texture texture = getTexture(textureState.textureHandle);
	textureState.numAnimations = texture.numAnimations;
	textureState.numFrames = texture.animations[textureState.currentAnimation].numFrames;
	textureState.currentFPS = texture.animations[textureState.currentAnimation].framesPerSecond;
	textureState.lastFrameTimeMS = getTimeMS();
	
	return textureState;
}

bool textureStateSetAnimation(TextureState *const pTextureState, const unsigned int nextAnimation) {
	if (pTextureState == NULL) {
		return false;
	} else if (nextAnimation >= pTextureState->numAnimations) {
		logf_message(WARNING, "Warning updating texture animation state: current animation index (%u) is not less than number of animations (%u).", pTextureState->currentAnimation, pTextureState->numAnimations);
		return false;
	}

	pTextureState->currentAnimation = nextAnimation;
	pTextureState->currentFrame = 0;
	pTextureState->lastFrameTimeMS = getTimeMS();

	return true;
}

int textureStateAnimate(TextureState *const pTextureState) {
	if (pTextureState == NULL) {
		return 0;
	}
	
	// Calculate the time difference between last frame change for this texture and current time, in seconds.
	const unsigned long long int currentTimeMS = getTimeMS();
	const unsigned long long int deltaTimeMS = currentTimeMS - pTextureState->lastFrameTimeMS;
	const double deltaTimeS = (double)deltaTimeMS / 1000.0;

	// Time in seconds * frames per second = number of frames to increment the frame counter by.
	const unsigned int frames = (unsigned int)(deltaTimeS * (double)pTextureState->currentFPS);

	// Update the current frame and last frame time of this texture.
	if (frames > 0) {
		pTextureState->currentFrame += frames;
		pTextureState->currentFrame %= pTextureState->numFrames;
		pTextureState->lastFrameTimeMS = currentTimeMS;
		return 2;
	}

	return 1;
}
