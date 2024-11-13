#ifndef RENDER_OBJECT_H
#define RENDER_OBJECT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "math/Box.h"
#include "math/Vector.h"
#include "util/string.h"
#include "vulkan/TextureState.h"
#include "vulkan/math/render_vector.h"

#include "render_config.h"

typedef enum RenderObjectQuadType {
	QUAD_TYPE_MAIN = 0,
	QUAD_TYPE_DEBUG = 1
} RenderObjectQuadType;

typedef struct QuadLoadInfo {
	
	RenderObjectQuadType quadType;
	
	Vector3D initPosition;
	
	BoxF quadDimensions;
	
	int initAnimation;
	
	// The color that the quad will have.
	Vector4F color;
	
} QuadLoadInfo;

typedef struct RenderObjectLoadInfo {
	
	// The texture that all quads controlled by the render object will use.
	// Only used for regular (i.e. not debug) render objects.
	String textureID;
	
	// The load infos for each quad.
	int quadCount;
	QuadLoadInfo *pQuadLoadInfos;
	
} RenderObjectLoadInfo;

extern const int renderHandleInvalid;

bool initRenderObjectManager(void);

bool terminateRenderObjectManager(void);

// Loads a render object into the scene, returns a handle to it.
[[deprecated]]
int loadRenderObject(const String textureID, const BoxF quadDimensions, const int numQuads, const Vector3D quadPositions[static numQuads]);

// Loads a render object into the scene.
// Returns a handle to the new render object.
int loadRenderObject2(const RenderObjectLoadInfo loadInfo);

// Unloads a render object, removing from the scene.
void unloadRenderObject(int *const pRenderHandle);

// Returns true if the render handle is both below the number of render object slots and positive, false otherwise.
bool validateRenderObjectHandle(const int renderHandle);

int renderObjectLoadDebugQuad(const int renderHandle, const Vector3D position, const BoxF dimensions, const Vector4F color);

// Returns true if the quad index is both below the max number of quads per render object and positive, false otherwise.
bool validateRenderObjectQuadIndex(const int quadIndex);

// Sets the position of a single quad associated with the render object.
bool renderObjectSetPosition(const int renderHandle, const int quadIndex, const Vector3D position);

int renderObjectGetTextureHandle(const int renderHandle, const int quadIndex);

// Animates each of the render object's quad's textures.
bool renderObjectAnimate(const int renderHandle);

// Returns the current animation of the render object's texture state which is referenced by the render handle.
// Returns 0 if the render object could not be accessed.
unsigned int renderObjectGetAnimation(const int renderHandle, const int quadIndex);

// Sets the current animation of the render object's texture state which is referenced by the render handle.
// Returns true if the animation was successfully updated, false otherwise.
bool renderObjectSetAnimation(const int renderHandle, const int quadIndex, const unsigned int nextAnimation);

int loadDebugRenderObject(const Vector4F color, const BoxF quadDimensions, const int quadCount, const Vector3D quadPositions[static const quadCount]);

#endif	// RENDER_OBJECT_H
