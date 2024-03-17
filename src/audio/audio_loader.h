#ifndef AUDIO_LOADER_H
#define AUDIO_LOADER_H

#include <stdbool.h>

#include "audio_data.h"

audio_data_t load_audio_file(const char *const filename);
bool unload_audio_file(audio_data_t *const audio_data_ptr);

#endif	// AUDIO_LOADER_H
