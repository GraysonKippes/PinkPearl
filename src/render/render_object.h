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

typedef struct transform_t {
	vector4F_t translation;
	vector4F_t scaling;
	vector4F_t rotation;
} transform_t;

extern const int render_handle_invalid;
extern const int render_handle_dangling;

bool init_render_object_manager(void);

int loadRenderObject(const DimensionsF quadDimensions, const transform_t transform, const string_t textureID);
void unloadRenderObject(int *const pRenderHandle);

// Returns true if the render handle is both below the number of render object slots, false otherwise.
bool validate_render_handle(int handle);

RenderTransform *getRenderObjTransform(const int render_handle);
TextureState *getRenderObjTexState(const int render_handle);

#endif	// RENDER_OBJECT_H
