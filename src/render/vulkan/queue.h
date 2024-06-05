#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stdint.h>

// This struct is used to describe which queues a shared resource will be shared between.
// If num_queue_families > 0 AND queue_familes != nullptr, then the sharing mode will be set to concurrent,
// 	and the the queue families will be passed to the object creation.
// Otherwise, the sharing mode will be set to exclusive, and no queue families will be passed to the object creation.
typedef struct queue_family_set_t {
	uint32_t num_queue_families;
	uint32_t *queue_families;
} queue_family_set_t;

// Empty queue family set, pass this into vulkan resource creation functions to specify that the resource will not be shared.
extern const queue_family_set_t queue_family_set_null;

bool is_queue_family_set_null(queue_family_set_t queue_family_set);

#endif	// QUEUE_H
