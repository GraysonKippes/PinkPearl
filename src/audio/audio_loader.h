#ifndef AUDIO_LOADER_H
#define AUDIO_LOADER_H

#include <stddef.h>

typedef float AudioSample;

typedef struct AudioData {
	unsigned int num_channels;
	unsigned int sample_rate;
	size_t num_samples;
	AudioSample *samples;
} AudioData;

typedef struct AudioTrack {
	bool loop;
	size_t current_position;
	AudioData data;
} AudioTrack;

AudioData load_audio_file(const char *const filename);

bool unload_audio_file(AudioData *const pAudioData);

#endif	// AUDIO_LOADER_H
