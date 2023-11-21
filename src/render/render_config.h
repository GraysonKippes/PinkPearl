#ifndef RENDER_CONFIG_H
#define RENDER_CONFIG_H

#include <stdint.h>

#define NUM_FRAMES_IN_FLIGHT 2

extern const uint32_t num_frames_in_flight;

#define NUM_RENDER_OBJECT_SLOTS 64

extern const uint32_t num_render_object_slots;

#define NUM_ROOM_RENDER_OBJECT_SLOTS 2

extern const uint32_t num_room_render_object_slots;

#endif	// RENDER_CONFIG_H
