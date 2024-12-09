#include "audio_mixer.h"

#include <math.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include "log/Logger.h"

static pthread_t audio_mixer_thread;
static atomic_bool audio_mixer_running = false;

// TODO - use mutexes.
static AudioTrack background_music_track;

AudioQueue audio_mixer_queue;

static void *audio_mixer_main(void *arg);
static void audio_mixer_mix(void);

// Call on main thread.
void init_audio_mixer(void) {
	
	logMsg(loggerAudio, LOG_LEVEL_VERBOSE, "Initializing audio mixer...");
	
	audio_mixer_queue = make_audio_queue();
	audio_mixer_set_music_track(load_audio_file("demo_dungeon.wav"));
	
	const int thread_create_result = pthread_create(&audio_mixer_thread, nullptr, audio_mixer_main, nullptr);
	if (thread_create_result != 0) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error initializing audio mixer: thread creation returned with code %i.", thread_create_result);
	}
	
	logMsg(loggerAudio, LOG_LEVEL_VERBOSE, "Done initializing audio mixer.");
}

// Call on main thread.
void terminate_audio_mixer(void) {
	
	logMsg(loggerAudio, LOG_LEVEL_VERBOSE, "Terminating audio mixer...");
	
	atomic_store(&audio_mixer_running, false);
	const int thread_join_result = pthread_join(audio_mixer_thread, nullptr);
	if (thread_join_result != 0) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error terminating audio mixer: thread joining returned with code %i.", thread_join_result);
	}
	
	destroy_audio_queue(&audio_mixer_queue);
	
	logMsg(loggerAudio, LOG_LEVEL_VERBOSE, "Done terminating audio mixer.");
}

void audio_mixer_set_music_track(const AudioData audio_data) {
	background_music_track.data = audio_data;
	background_music_track.loop = true;
	background_music_track.current_position = 0;
}

static void *audio_mixer_main(void *arg) {
	(void)arg;
	atomic_store(&audio_mixer_running, true);
	while (atomic_load(&audio_mixer_running)) {
		audio_mixer_mix();
	}
	return nullptr;
}

static void audio_mixer_mix(void) {
	
	static const unsigned int excess_node_limit = 8;	// TODO - move to audio config
	if (atomic_load(&audio_mixer_queue.num_excess_nodes) < excess_node_limit) {
	
		// Create the next node.
		atomic_store(&audio_mixer_queue.pTailNode->pNextNode, (_Atomic AudioQueueNode *)new_audio_queue_node());
		_Atomic AudioQueueNode *pNextNode = atomic_load(&audio_mixer_queue.pTailNode->pNextNode);
		
		// Mix audio data into new node.
		if (background_music_track.current_position + audio_buffer_length >= background_music_track.data.num_samples) {
			size_t mix_index = background_music_track.current_position;
			for (size_t i = 0; i < audio_buffer_length; ++i) {
				pNextNode->samples[i] = background_music_track.data.samples[mix_index];
				if (++mix_index >= background_music_track.data.num_samples) {
					mix_index -= background_music_track.data.num_samples;
				}
			}
			background_music_track.current_position = mix_index;
		}
		else {
			//memcpy_s(pNextNode->samples, audio_buffer_length * sizeof(AudioSample), &background_music_track.data.samples[background_music_track.current_position], audio_buffer_length * sizeof(AudioSample));
			for (size_t i = 0; i < audio_buffer_length; i++) {
				pNextNode->samples[i] = background_music_track.data.samples[background_music_track.current_position + i];
			}
			background_music_track.current_position += audio_buffer_length;
		}
		
		// Publish the new node.
		atomic_store(&audio_mixer_queue.pTailNode, audio_mixer_queue.pTailNode->pNextNode);
		atomic_fetch_add(&audio_mixer_queue.num_excess_nodes, 1);
	}
	
	// Remove already-processed nodes.
	while (audio_mixer_queue.pHeadNode != audio_mixer_queue.pNeckNode) {
		AudioQueueNode *const temp_node_ptr = audio_mixer_queue.pHeadNode;
		audio_mixer_queue.pHeadNode = (AudioQueueNode *)audio_mixer_queue.pHeadNode->pNextNode;
		destroy_audio_queue_node(temp_node_ptr);
	}
}