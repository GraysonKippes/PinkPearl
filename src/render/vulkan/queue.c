#include "queue.h"

#include <stddef.h>

const queue_family_set_t queue_family_set_null = {
	.num_queue_families = 0,
	.queue_families = nullptr
};

bool is_queue_family_set_null(queue_family_set_t queue_family_set) {
	return queue_family_set.num_queue_families == 0 || queue_family_set.queue_families == nullptr;
}
