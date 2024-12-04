#ifndef RENDER_MANAGER_H
#define RENDER_MANAGER_H

#include <stdint.h>

#include "area_render_state.h"

// TYPE DEFINITIONS

typedef enum RenderObjectQuadType {
	QUAD_TYPE_MAIN = 0,
	QUAD_TYPE_GUI = 1,
	QUAD_TYPE_WIREFRAME = 2
} RenderObjectQuadType;

typedef struct QuadLoadInfo {
	
	RenderObjectQuadType quadType;
	
	Vector3D initPosition;
	
	BoxF quadDimensions;
	
	int32_t initAnimation;
	
	int32_t initCell;
	
	// The color that the quad will have.
	Vector4F color;
	
} QuadLoadInfo;

typedef struct RenderObjectLoadInfo {
	
	// The texture that all quads controlled by the render object will use.
	// Only used for regular (i.e. not debug) render objects.
	String textureID;
	
	// The load infos for each quad.
	int32_t quadCount;
	QuadLoadInfo *pQuadLoadInfos;
	
} RenderObjectLoadInfo;

// GLOBAL VARIABLES

extern const Vector4F COLOR_WHITE;
extern const Vector4F COLOR_BLACK;
extern const Vector4F COLOR_RED;
extern const Vector4F COLOR_GREEN;
extern const Vector4F COLOR_BLUE;
extern const Vector4F COLOR_YELLOW;
extern const Vector4F COLOR_TEAL;
extern const Vector4F COLOR_PURPLE;
extern const Vector4F COLOR_PINK;

extern AreaRenderState globalAreaRenderState;

// GENERAL RENDER MANAGER FUNCTIONS

// Initializes the render manager.
void initRenderManager(void);

// Terminates the render manager.
void terminateRenderManager(void);

// Synchronizes the render manager with the game layer.
void tickRenderManager(void);

// Renders a single frame and polls GLFW for events.
void renderFrame(const float timeDelta, const bool paused);

// RENDER OBJECT INTERFACE

// Loads a render object, AKA a collection of quads being rendered that can be managed through a single handle.
int32_t loadRenderObject(const RenderObjectLoadInfo loadInfo);

// Unloads a render object.
void unloadRenderObject(int32_t *const pRenderObjectHandle);

// Loads a render object displaying text.
int32_t loadRenderText(const String text, const Vector3D position, const Vector4F color);

// Writes formatted text to a render object.
void writeRenderText(const int32_t handle, const char *const pFormat, ...);

// Checks whether a render object handle is valid or not.
bool validateRenderObjectHandle(const int32_t handle);

// Checks whether a render object handle is valid and the object it handles does exist.
bool renderObjectExists(const int32_t handle);

// Loads a quad into a render object.
int32_t renderObjectLoadQuad(const int32_t handle, const QuadLoadInfo loadInfo);

// Checks whether a render object quad index is valid or not.
bool validateRenderObjectQuadIndex(const int32_t quadIndex);

// Checks whether a quad belonging to a render object exists or not.
bool renderObjectQuadExists(const int32_t handle, const int32_t quadIndex);

// Sets the position of a single quad associated with the render object.
void renderObjectSetPosition(const int32_t handle, const int32_t quadIndex, const Vector3D position);

int renderObjectGetTextureHandle(const int renderHandle, const int quadIndex);

// Animates each of the render object's quad's textures.
void renderObjectAnimate(const int32_t handle);

// Returns the current animation of the render object's texture state which is referenced by the render handle.
// Returns 0 if the render object could not be accessed.
unsigned int renderObjectGetAnimation(const int renderHandle, const int quadIndex);

// Sets the current animation of the render object's texture state which is referenced by the render handle.
// Returns true if the animation was successfully updated, false otherwise.
bool renderObjectSetAnimation(const int renderHandle, const int quadIndex, const unsigned int nextAnimation);

#endif	// RENDER_MANAGER_H