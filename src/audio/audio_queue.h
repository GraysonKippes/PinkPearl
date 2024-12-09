#ifndef AUDIO_QUEUE_H
#define AUDIO_QUEUE_H

#include <stdatomic.h>
#include <stddef.h>
#include "audio_config.h"
#include "audio_loader.h"

extern const size_t audio_queue_buffer_length;

typedef struct AudioQueueNode {
	AudioSample samples[AUDIO_BUFFER_LENGTH];
	_Atomic struct AudioQueueNode *pNextNode;
} AudioQueueNode;

AudioQueueNode *new_audio_queue_node(void);
bool destroy_audio_queue_node(AudioQueueNode *const audio_queue_node_ptr);

typedef struct AudioQueue {
	
	AudioQueueNode *pHeadNode;	// Points to first node in underlying list.
	_Atomic AudioQueueNode *pNeckNode;	// Points to first unused node in underlying list.
	_Atomic AudioQueueNode *pTailNode;	// Points to last node in underlying list.
	
	// Number of nodes after neck node.
	atomic_uint num_excess_nodes;
	
} AudioQueue;

AudioQueue make_audio_queue(void);
bool destroy_audio_queue(AudioQueue *const pAudioQueue);

#endif	// AUDIO_QUEUE_H
