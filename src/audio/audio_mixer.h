#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#include "audio_data.h"
#include "audio_queue.h"

extern audio_queue_t audio_mixer_queue;

void init_audio_mixer(void);
void terminate_audio_mixer(void);
void audio_mixer_set_music_track(const audio_data_t audio_data);

#endif	// AUDIO_MIXER_H
