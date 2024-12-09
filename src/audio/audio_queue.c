#include "audio_queue.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

AudioQueueNode *new_audio_queue_node(void) {
	AudioQueueNode *audio_queue_node_ptr = malloc(sizeof(AudioQueueNode));
	atomic_init(&audio_queue_node_ptr->pNextNode, nullptr);
	return audio_queue_node_ptr;
}

bool destroy_audio_queue_node(AudioQueueNode *const audio_queue_node_ptr) {
	if (!audio_queue_node_ptr) {
		return false;
	}
	atomic_store(&audio_queue_node_ptr->pNextNode, nullptr);
	return true;
}

AudioQueue make_audio_queue(void) {

	AudioQueue audio_queue = { };
	AudioQueueNode *init_node_ptr = new_audio_queue_node();
	audio_queue.pHeadNode = init_node_ptr;
	atomic_init(&audio_queue.pNeckNode, (_Atomic AudioQueueNode *)init_node_ptr);
	atomic_init(&audio_queue.pTailNode, (_Atomic AudioQueueNode *)init_node_ptr);
	atomic_init(&audio_queue.num_excess_nodes, 0);

	return audio_queue;
}

bool destroy_audio_queue(AudioQueue *const pAudioQueue) {
	assert(pAudioQueue);
	
	AudioQueueNode *node_ptr = pAudioQueue->pHeadNode;
	while (node_ptr != nullptr) {
		AudioQueueNode *temp_node_ptr = node_ptr;
		node_ptr = (AudioQueueNode *)atomic_load(&node_ptr->pNextNode);
		destroy_audio_queue_node(temp_node_ptr);
	}
	
	pAudioQueue->pHeadNode = nullptr;
	atomic_init(&pAudioQueue->pNeckNode, nullptr);
	atomic_init(&pAudioQueue->pTailNode, nullptr);
	atomic_init(&pAudioQueue->num_excess_nodes, 0);
	
	return true;
}