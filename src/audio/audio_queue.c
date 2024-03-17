#include "audio_queue.h"

#include <stddef.h>
#include <stdlib.h>

audio_queue_node_t *new_audio_queue_node(void) {
	audio_queue_node_t *audio_queue_node_ptr = malloc(sizeof(audio_queue_node_t));
	atomic_init(&audio_queue_node_ptr->next_node_ptr, NULL);
	return audio_queue_node_ptr;
}

bool destroy_audio_queue_node(audio_queue_node_t *const audio_queue_node_ptr) {

	if (audio_queue_node_ptr == NULL) {
		return false;
	}

	atomic_store(&audio_queue_node_ptr->next_node_ptr, NULL);

	return true;
}

audio_queue_t make_audio_queue(void) {

	audio_queue_t audio_queue;
	audio_queue_node_t *init_node_ptr = new_audio_queue_node();
	audio_queue.head_node_ptr = init_node_ptr;
	atomic_init(&audio_queue.neck_node_ptr, (_Atomic audio_queue_node_t *)init_node_ptr);
	atomic_init(&audio_queue.tail_node_ptr, (_Atomic audio_queue_node_t *)init_node_ptr);
	audio_queue.num_excess_nodes = 0;

	return audio_queue;
}
