#ifndef AUDIO_QUEUE_H
#define AUDIO_QUEUE_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

#include "audio_config.h"
#include "audio_data.h"

extern const size_t audio_queue_buffer_length;

typedef struct audio_queue_node_t {
	audio_sample_t samples[AUDIO_BUFFER_LENGTH];
	_Atomic struct audio_queue_node_t *next_node_ptr;
} audio_queue_node_t;

audio_queue_node_t *new_audio_queue_node(void);
bool destroy_audio_queue_node(audio_queue_node_t *const audio_queue_node_ptr);

typedef struct audio_queue_t {
	
	audio_queue_node_t *head_node_ptr;	// Points to first node in underlying list.
	_Atomic audio_queue_node_t *neck_node_ptr;	// Points to first unused node in underlying list.
	_Atomic audio_queue_node_t *tail_node_ptr;	// Points to last node in underlying list.
	
	// Number of nodes after neck node.
	atomic_uint num_excess_nodes;
	
} audio_queue_t;

audio_queue_t make_audio_queue(void);
bool destroy_audio_queue(audio_queue_t *const audio_queue_ptr);

#endif	// AUDIO_QUEUE_H
