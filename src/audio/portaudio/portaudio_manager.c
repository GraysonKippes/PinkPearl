#include "portaudio_manager.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <portaudio/portaudio.h>

#include "audio/audio_config.h"
#include "audio/audio_mixer.h"
#include "log/Logger.h"

static const int num_input_channels = 0;
static const int num_output_channels = NUM_AUDIO_CHANNELS;
static const int audio_sample_rate = 44100;

static PaStream *audio_stream = nullptr;

static int portaudio_stream_callback(const void *input_buffer, void *output_buffer, unsigned long int frame_count, const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags status_flags, void *user_data) {

	(void)input_buffer;
	(void)time_info;
	(void)status_flags;
	(void)user_data;

	audio_sample_t *const output = (audio_sample_t *)output_buffer;
	audio_queue_node_t *neck_node_ptr = (audio_queue_node_t *)atomic_load(&audio_mixer_queue.neck_node_ptr);
	const audio_queue_node_t *const tail_node_ptr = (audio_queue_node_t *)atomic_load(&audio_mixer_queue.tail_node_ptr);
	
	if (neck_node_ptr != tail_node_ptr) {
		for (unsigned long int i = 0; i < frame_count * num_output_channels; ++i) {
			output[i] = neck_node_ptr->next_node_ptr->samples[i];
		}
		atomic_store(&audio_mixer_queue.neck_node_ptr, neck_node_ptr->next_node_ptr);
		atomic_fetch_sub(&audio_mixer_queue.num_excess_nodes, 1);
	}
	else {
		memset(output, 0, frame_count * sizeof(audio_sample_t));
	}
	return 0;
}

void init_portaudio(void) {
	
	logMsg(loggerAudio, LOG_LEVEL_VERBOSE, "Initializing PortAudio...");
	
	const int init_result = Pa_Initialize();
	if (init_result != paNoError) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error initializing PortAudio: initialization failed (result code: \"%s\").", Pa_GetErrorText(init_result));
		return;
	}
	
	const int stream_open_result = Pa_OpenDefaultStream(&audio_stream, num_input_channels, num_output_channels, paFloat32, audio_sample_rate, num_audio_frames_per_buffer, portaudio_stream_callback, nullptr);
	if (stream_open_result != paNoError) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error initializing PortAudio: failed to open audio stream (result code: \"%s\").", Pa_GetErrorText(stream_open_result));
		return;
	}

	const int stream_start_result = Pa_StartStream(audio_stream);
	if (stream_start_result != paNoError) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error initializing PortAudio: failed to start audio stream (result code: \"%s\").", Pa_GetErrorText(stream_start_result));
		return;
	}

	logMsg(loggerAudio, LOG_LEVEL_VERBOSE, "Done initializing PortAudio.");
}

void terminate_portaudio(void) {
	
	logMsg(loggerAudio, LOG_LEVEL_VERBOSE, "Terminating PortAudio...");
	
	const int stream_close_result = Pa_CloseStream(audio_stream);
	if (stream_close_result != paNoError) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error terminating PortAudio: failed to close audio stream (result code: \"%s\").", Pa_GetErrorText(stream_close_result));
		return;
	}

	const int terminate_result = Pa_Terminate();
	if (terminate_result != paNoError) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error terminating PortAudio: termination failed (result code: \"%s\").", Pa_GetErrorText(terminate_result));
		return;
	}
	
	logMsg(loggerAudio, LOG_LEVEL_VERBOSE, "Done terminating PortAudio.");
}