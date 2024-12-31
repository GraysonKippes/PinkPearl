#include "TextureState.h"

#include <stddef.h>

#include "log/Logger.h"
#include "util/Allocation.h"
#include "util/time.h"

#include "texture.h"
#include "texture_manager.h"

// TODO - change to const global variable
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
	if (stringIsNull(textureID)) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating texture state: given texture ID is nullptr.");
		return nullTextureState();
	}
	
	TextureState textureState = nullTextureState();
	
	textureState.textureHandle = findTexture(textureID);
	if (!validateTextureHandle(textureState.textureHandle)) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating texture state: could not find texture \"%s\".", textureID.pBuffer);
		return (TextureState){ };
	}
	
	Texture texture = getTexture(textureState.textureHandle);
	textureState.numAnimations = texture.numAnimations;
	textureState.startCell = texture.animations[textureState.currentAnimation].startCell;
	textureState.numFrames = texture.animations[textureState.currentAnimation].numFrames;
	textureState.currentFPS = texture.animations[textureState.currentAnimation].framesPerSecond;
	textureState.lastFrameTimeMS = getTimeMS();
	
	return textureState;
}

TextureState newTextureState2(const int32_t textureHandle) {
	if (!validateTextureHandle(textureHandle)) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating texture state: texture %i does not exist.", textureHandle);
		return nullTextureState();
	}
	
	Texture texture = getTexture(textureHandle);
	
	return (TextureState){
		.textureHandle = textureHandle,
		.numAnimations = texture.numAnimations,
		.currentAnimation = 0,
		.startCell = texture.animations[0].startCell,
		.numFrames = texture.animations[0].numFrames,
		.currentFPS = texture.animations[0].framesPerSecond,
		.currentFrame = 0,
		.lastFrameTimeMS = getTimeMS()
	};
}

bool textureStateSetAnimation(TextureState *const pTextureState, const unsigned int nextAnimation) {
	if (!pTextureState) {
		return false;
	} else if (nextAnimation >= pTextureState->numAnimations) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Updating texture animation state: next animation index (%u) is not less than number of animations (%u).", nextAnimation, pTextureState->numAnimations);
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
	if (!pTextureState) {
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
