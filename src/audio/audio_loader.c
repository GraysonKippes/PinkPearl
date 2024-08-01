#include "audio_loader.h"

#define DR_WAV_IMPLEMENTATION
#include <dr_audio/dr_wav.h>

#include "config.h"
#include "log/Logger.h"
#include "util/allocate.h"

#define AUDIO_PATH (RESOURCE_PATH "assets/audio/music/demo_dungeon.wav")

audio_data_t load_audio_file(const char *const filename) {
	
	audio_data_t audio_data = { };
	if (!filename) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error loading audio file: filename is null.");
		return audio_data;
	}
	
	audio_data.samples = drwav_open_file_and_read_pcm_frames_f32(AUDIO_PATH, &audio_data.num_channels, &audio_data.sample_rate, (drwav_uint64 *)&audio_data.num_samples, nullptr);
	
	if (!audio_data.samples) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error loading audio file \"%s\": samples is null.", filename);
	}
	
	if (audio_data.num_samples == 0) {
		logMsg(loggerAudio, LOG_LEVEL_ERROR, "Error loading audio file \"%s\": number of samples is zero.", filename);
	}
	
	return audio_data;
}

bool unload_audio_file(audio_data_t *const audio_data_ptr) {

	if (!audio_data_ptr) {
		return false;
	}

	free(audio_data_ptr->samples);
	audio_data_ptr->num_samples = 0;
	audio_data_ptr->samples = nullptr;

	return true;
}
