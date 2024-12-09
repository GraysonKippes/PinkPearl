#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#include "audio_loader.h"
#include "audio_queue.h"

extern AudioQueue audio_mixer_queue;

void init_audio_mixer(void);
void terminate_audio_mixer(void);
void audio_mixer_set_music_track(const AudioData audio_data);

#endif	// AUDIO_MIXER_H
