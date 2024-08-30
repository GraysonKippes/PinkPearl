#include "Draw.h"

#include <stdint.h>

typedef struct DrawInfo {
	
	/* Indirect draw command parameters */
	
	uint32_t indexCount;
	uint32_t instanceCount;
	uint32_t firstIndex;
	int32_t vertexOffset;
	uint32_t firstInstance;
	
	/* Additional draw parameters */
	
	int32_t modelID;
	uint32_t imageIndex;
	
} DrawInfo;

static const uint32_t indirectDrawStride = sizeof(DrawInfo);


