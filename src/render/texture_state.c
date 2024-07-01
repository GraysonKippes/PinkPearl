#include "texture_state.h"

#include <stddef.h>

#include "log/logging.h"
#include "util/allocate.h"
#include "util/time.h"
#include "vulkan/texture.h"
#include "vulkan/texture_manager.h"

TextureState nullTextureState(void) {
	return (TextureState){
		.textureHandle = textureHandleMissing,
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
	
	if (stringIsNull(textureID)) {
		logMsg(LOG_LEVEL_ERROR, "Error finding loaded texture: given texture ID is nullptr.");
		return textureState;
	}
	
	textureState.textureHandle = findTexture(textureID);
	if (!validateTextureHandle(textureState.textureHandle)) {
		textureState.textureHandle = textureHandleMissing;
		return textureState;
	}
	
	Texture texture = getTexture(textureState.textureHandle);
	textureState.numAnimations = texture.numAnimations;
	textureState.startCell = texture.animations[textureState.currentAnimation].startCell;
	textureState.numFrames = texture.animations[textureState.currentAnimation].numFrames;
	textureState.currentFPS = texture.animations[textureState.currentAnimation].framesPerSecond;
	textureState.lastFrameTimeMS = getTimeMS();
	
	return textureState;
}

bool textureStateSetAnimation(TextureState *const pTextureState, const unsigned int nextAnimation) {
	if (pTextureState == nullptr) {
		return false;
	} else if (nextAnimation >= pTextureState->numAnimations) {
		logMsgF(LOG_LEVEL_WARNING, "Warning updating texture animation state: current animation index (%u) is not less than number of animations (%u).", pTextureState->currentAnimation, pTextureState->numAnimations);
		return false;
	}

	const TextureAnimation textureAnimation = getTexture(pTextureState->textureHandle).animations[nextAnimation];

	pTextureState->currentAnimation = nextAnimation;
	pTextureState->startCell = textureAnimation.startCell;
	pTextureState->numFrames = textureAnimation.numFrames;
	pTextureState->currentFPS = textureAnimation.framesPerSecond;
	pTextureState->currentFrame = 0;
	pTextureState->lastFrameTimeMS = getTimeMS();

	return true;
}

int textureStateAnimate(TextureState *const pTextureState) {
	if (pTextureState == nullptr) {
		return 0;
	}
	
	if (pTextureState->currentFPS == 0) {
		return 1;
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
