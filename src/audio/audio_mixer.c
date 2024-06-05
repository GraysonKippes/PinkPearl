#include "audio_mixer.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <pthread.h>

#include "log/logging.h"

#include "audio_loader.h"

static pthread_t audio_mixer_thread;
static atomic_bool audio_mixer_running = false;

// TODO - use mutexes.
static audio_track_t background_music_track;

audio_queue_t audio_mixer_queue;

static void *audio_mixer_main(void *arg);
static void audio_mixer_mix(void);

// Call on main thread.
void init_audio_mixer(void) {
	
	log_message(VERBOSE, "Initializing audio mixer...");
	
	audio_mixer_queue = make_audio_queue();
	audio_mixer_set_music_track(load_audio_file("demo_dungeon.wav"));
	
	const int thread_create_result = pthread_create(&audio_mixer_thread, nullptr, audio_mixer_main, nullptr);
	if (thread_create_result != 0) {
		logf_message(ERROR, "Error initializing audio mixer: thread creation returned with code %i.", thread_create_result);
	}
	
	log_message(VERBOSE, "Done initializing audio mixer.");
}

// Call on main thread.
void terminate_audio_mixer(void) {
	
	log_message(VERBOSE, "Terminating audio mixer...");
	
	atomic_store(&audio_mixer_running, false);
	const int thread_join_result = pthread_join(audio_mixer_thread, nullptr);
	if (thread_join_result != 0) {
		logf_message(ERROR, "Error terminating audio mixer: thread joining returned with code %i.", thread_join_result);
	}
	
	destroy_audio_queue(&audio_mixer_queue);
	
	log_message(VERBOSE, "Done terminating audio mixer.");
}

void audio_mixer_set_music_track(const audio_data_t audio_data) {
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
		atomic_store(&audio_mixer_queue.tail_node_ptr->next_node_ptr, (_Atomic audio_queue_node_t *)new_audio_queue_node());
		_Atomic audio_queue_node_t *next_node_ptr = atomic_load(&audio_mixer_queue.tail_node_ptr->next_node_ptr);
		
		// Mix audio data into new node.
		if (background_music_track.current_position + audio_buffer_length >= background_music_track.data.num_samples) {
			size_t mix_index = background_music_track.current_position;
			for (size_t i = 0; i < audio_buffer_length; ++i) {
				next_node_ptr->samples[i] = background_music_track.data.samples[mix_index];
				if (++mix_index >= background_music_track.data.num_samples) {
					mix_index -= background_music_track.data.num_samples;
				}
			}
			background_music_track.current_position = mix_index;
		}
		else {
			//memcpy_s(next_node_ptr->samples, audio_buffer_length * sizeof(audio_sample_t), &background_music_track.data.samples[background_music_track.current_position], audio_buffer_length * sizeof(audio_sample_t));
			for (size_t i = 0; i < audio_buffer_length; i++) {
				next_node_ptr->samples[i] = background_music_track.data.samples[background_music_track.current_position + i];
			}
			background_music_track.current_position += audio_buffer_length;
		}
		
		// Publish the new node.
		atomic_store(&audio_mixer_queue.tail_node_ptr, audio_mixer_queue.tail_node_ptr->next_node_ptr);
		atomic_fetch_add(&audio_mixer_queue.num_excess_nodes, 1);
	}
	
	// Remove already-processed nodes.
	while (audio_mixer_queue.head_node_ptr != audio_mixer_queue.neck_node_ptr) {
		audio_queue_node_t *const temp_node_ptr = audio_mixer_queue.head_node_ptr;
		audio_mixer_queue.head_node_ptr = (audio_queue_node_t *)audio_mixer_queue.head_node_ptr->next_node_ptr;
		destroy_audio_queue_node(temp_node_ptr);
	}
}