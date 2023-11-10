#include "queue.h"

#include <stddef.h>

const queue_family_set_t queue_family_set_null = {
	.m_num_queue_families = 0,
	.m_queue_families = NULL
};

bool is_queue_family_set_null(queue_family_set_t queue_family_set) {
	return queue_family_set.m_num_queue_families == 0 
		|| queue_family_set.m_queue_families == NULL;
}
