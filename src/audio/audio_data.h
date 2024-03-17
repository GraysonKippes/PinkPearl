#ifndef AUDIO_DATA_H
#define AUDIO_DATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef float audio_sample_t;

typedef struct audio_data_t {
	unsigned int num_channels;
	unsigned int sample_rate;
	size_t num_samples;
	audio_sample_t *samples;
} audio_data_t;

typedef struct audio_track_t {
	bool loop;
	size_t current_position;
	audio_data_t data;
} audio_track_t;

#endif	// AUDIO_DATA_H
