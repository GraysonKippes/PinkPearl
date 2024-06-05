#ifndef RENDER_OBJECT_H
#define RENDER_OBJECT_H

#include <stdbool.h>
#include <stdint.h>

#include "math/dimensions.h"
#include "util/string.h"
#include "vulkan/math/render_vector.h"

#include "render_config.h"
#include "texture_state.h"

/* RENDER OBJECT
*	quad ID
* 	transform
*	texture state
*/

extern const int renderHandleInvalid;

bool initRenderObjectManager(void);
bool terminateRenderObjectManager(void);

// Loads a render object into the scene, returns a handle to it.
int loadRenderObject(const DimensionsF quadDimensions, const Transform transform, const String textureID);

// Unloads a render object, removing from the scene.
void unloadRenderObject(int *const pRenderHandle);

// Returns true if the render handle is both below the number of render object slots, false otherwise.
bool validateRenderHandle(int renderHandle);

RenderTransform *getRenderObjTransform(const int renderHandle);

TextureState *getRenderObjTexState(const int renderHandle);

// Returns the current animation of the render object's texture state which is referenced by the render handle.
// Returns 0 if the render object could not be accessed.
unsigned int renderObjectGetAnimation(const int renderHandle);

// Sets the current animation of the render object's texture state which is referenced by the render handle.
// Returns true if the animation was successfully updated, false otherwise.
bool renderObjectSetAnimation(const int renderHandle, const unsigned int nextAnimation);

#endif	// RENDER_OBJECT_H
