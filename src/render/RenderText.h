#ifndef RENDER_TEXT_H
#define RENDER_TEXT_H

#include "math/Vector.h"
#include "util/String.h"

typedef struct RenderTextObject_T *RenderTextObject;

RenderTextObject loadRenderTextObject(const String text, const Vector3D position, const Vector3D color);

void unloadRenderTextObject(RenderTextObject *const pRenderTextObject);

#endif // RENDER_TEXT_H