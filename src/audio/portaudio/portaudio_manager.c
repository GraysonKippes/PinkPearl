#include "portaudio_manager.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <portaudio/portaudio.h>

#include "audio/audio_config.h"
#include "audio/audio_mixer.h"
#include "log/Logger.h"

#define INPUT_CHANNEL_COUNT 0
#define OUTPUT_CHANNEL_COUNT 2
#define AUDIO_SAMPLE_RATE 44100

static const int num_input_channels = 0;
static const int num_output_channels = NUM_AUDIO_CHANNELS;
static const int audio_sample_rate = 44100;

static PaStream *pAudioStream = nullptr;

static int audioStreamCallback(const void *pInputBuffer, void *pOutputBuffer, unsigned long int frameCount, const PaStreamCallbackTimeInfo *pTimeInfo, PaStreamCallbackFlags statusFlags, void *pUserData) {
	(void)pInputBuffer;
	(void)pTimeInfo;
	(void)statusFlags;
	(void)pUserData;

	AudioSample *const pOut = (AudioSample *)pOutputBuffer;
	AudioQueueNode *pNeckNode = (AudioQueueNode *)atomic_load(&audio_mixer_queue.pNeckNode);
	const AudioQueueNode *const pTailNode = (AudioQueueNode *)atomic_load(&audio_mixer_queue.pTailNode);
	
	if (pNeckNode != pTailNode) {
		for (unsigned long int i = 0; i < frameCount * num_output_channels; ++i) {
			pOut[i] = pNeckNode->pNextNode->samples[i];
		}
		atomic_store(&audio_mixer_queue.pNeckNode, pNeckNode->pNextNode);
		atomic_fetch_sub(&audio_mixer_queue.num_excess_nodes, 1);
	} else {
		memset(pOut, 0, frameCount * sizeof(AudioSample));
	}
	return paContinue;
}

void init_portaudio(void) {
	logMsg(loggerAudio, LOG_LEVEL_VERBOSE, "Initializing PortAudio...");
	
	const int init_result = Pa_Initialize();
	if (init_result != paNoError) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error initializing PortAudio: initialization failed (result code: \"%s\").", Pa_GetErrorText(init_result));
		return;
	}
	
	const int stream_open_result = Pa_OpenDefaultStream(&pAudioStream, num_input_channels, num_output_channels, paFloat32, audio_sample_rate, num_audio_frames_per_buffer, audioStreamCallback, nullptr);
	if (stream_open_result != paNoError) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error initializing PortAudio: failed to open audio stream (result code: \"%s\").", Pa_GetErrorText(stream_open_result));
		return;
	}

	const int stream_start_result = Pa_StartStream(pAudioStream);
	if (stream_start_result != paNoError) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error initializing PortAudio: failed to start audio stream (result code: \"%s\").", Pa_GetErrorText(stream_start_result));
		return;
	}

	logMsg(loggerAudio, LOG_LEVEL_VERBOSE, "Done initializing PortAudio.");
}

void terminate_portaudio(void) {
	logMsg(loggerAudio, LOG_LEVEL_VERBOSE, "Terminating PortAudio...");
	
	const PaError stopStreamResult = Pa_StopStream(pAudioStream);
	if (stopStreamResult != paNoError) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error terminating PortAudio: failed to stop audio stream (result code: \"%s\").", Pa_GetErrorText(stopStreamResult));
		return;
	}
	
	const PaError closeStreamResult = Pa_CloseStream(pAudioStream);
	if (closeStreamResult != paNoError) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error terminating PortAudio: failed to close audio stream (result code: \"%s\").", Pa_GetErrorText(closeStreamResult));
		return;
	}

	const PaError terminatePortAudioResult = Pa_Terminate();
	if (terminatePortAudioResult != paNoError) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error terminating PortAudio: termination failed (result code: \"%s\").", Pa_GetErrorText(terminatePortAudioResult));
		return;
	}
	
	logMsg(loggerAudio, LOG_LEVEL_VERBOSE, "Done terminating PortAudio.");
}