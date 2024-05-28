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

extern const int render_handle_invalid;
extern const int render_handle_dangling;

bool init_render_object_manager(void);

int loadRenderObject(const DimensionsF quadDimensions, const Transform transform, const String textureID);
void unloadRenderObject(int *const pRenderHandle);

// Returns true if the render handle is both below the number of render object slots, false otherwise.
bool validateRenderHandle(int handle);

RenderTransform *getRenderObjTransform(const int render_handle);
TextureState *getRenderObjTexState(const int render_handle);

#endif	// RENDER_OBJECT_H
