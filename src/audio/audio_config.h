#ifndef AUDIO_CONFIG_H
#define AUDIO_CONFIG_H

#include <stddef.h>

#define NUM_AUDIO_CHANNELS 2
extern const unsigned int num_audio_channels;

#define NUM_AUDIO_FRAMES_PER_BUFFER 4096
extern const size_t num_audio_frames_per_buffer;

#define AUDIO_BUFFER_LENGTH (NUM_AUDIO_CHANNELS * NUM_AUDIO_FRAMES_PER_BUFFER)
extern const size_t audio_buffer_length;

#endif	// AUDIO_CONFIG_H