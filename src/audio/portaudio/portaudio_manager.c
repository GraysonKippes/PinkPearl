#include "portaudio_manager.h"

#include <stddef.h>
#include <stdint.h>

#include <portaudio/portaudio.h>

#include "log/logging.h"

static const int audio_sample_rate = 44100;
static const unsigned long int frames_per_buffer = 256;

static PaStream *audio_stream = NULL;

static int portaudio_stream_callback(const void *input_buffer, void *output_buffer, unsigned long int frame_count, const PaStreamCallbackTimeInfo *time_info, PaStreamCallbackFlags status_flags, void *user_data) {
	for (unsigned long int i = 0; i < frame_count; ++i) {
		((int32_t *)output_buffer)[i] = 0;
	}
	return 0;
}

void init_portaudio(void) {
	
	log_message(VERBOSE, "Initializing PortAudio...");
	
	const int init_result = Pa_Initialize();
	if (init_result != paNoError) {
		logf_message(ERROR, "Error initializing PortAudio: initialization failed (result code: \"%s\").", Pa_GetErrorText(init_result));
		return;
	}
	
	const int stream_open_result = Pa_OpenDefaultStream(&audio_stream, 0, 2, paInt32, audio_sample_rate, frames_per_buffer, portaudio_stream_callback, NULL);
	if (stream_open_result != paNoError) {
		logf_message(ERROR, "Error initializing PortAudio: failed to open audio stream (result code: \"%s\").", Pa_GetErrorText(stream_open_result));
		return;
	}

	const int stream_start_result = Pa_StartStream(audio_stream);
	if (stream_start_result != paNoError) {
		logf_message(ERROR, "Error initializing PortAudio: failed to start audio stream (result code: \"%s\").", Pa_GetErrorText(stream_start_result));
		return;
	}

	log_message(VERBOSE, "Done initializing PortAudio.");
}

void terminate_portaudio(void) {
	
	log_message(VERBOSE, "Terminating PortAudio...");
	
	const int stream_close_result = Pa_CloseStream(audio_stream);
	if (stream_close_result != paNoError) {
		logf_message(ERROR, "Error terminating PortAudio: failed to close audio stream (result code: \"%s\").", Pa_GetErrorText(stream_close_result));
		return;
	}

	const int terminate_result = Pa_Terminate();
	if (terminate_result != paNoError) {
		logf_message(ERROR, "Error terminating PortAudio: termination failed (result code: \"%s\").", Pa_GetErrorText(terminate_result));
		return;
	}
	
	log_message(VERBOSE, "Done terminating PortAudio.");
}
