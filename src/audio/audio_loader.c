#include "audio_loader.h"

#define DR_WAV_IMPLEMENTATION
#include <dr_audio/dr_wav.h>

#include "config.h"
#include "log/logging.h"
#include "util/allocate.h"

#define AUDIO_PATH (RESOURCE_PATH "assets/audio/demo_dungeon.wav")

audio_data_t load_audio_file(const char *const filename) {
	
	audio_data_t audio_data = {
		.num_samples = 0,
		.samples = NULL
	};
	
	drwav wav_data;
	if (!drwav_init_file(&wav_data, audio_data, NULL)) {
		log_message(ERROR, "Error loading audio file: file loading failed.");
		return audio_data;
	}
	
	const size_t num_samples = wav_data.totalPCMFrameCount * wav_data.channels;
	if (!allocate((void **)&audio_data.samples, num_samples, sizeof(int32_t))) {
		log_message(ERROR, "Error loading audio file: failed to allocate audio data samples pointer-array.");
		return audio_data;
	}
	
	audio_data.num_samples = drwav_read_pcm_frames_s32(&wav_data, wav_data.totalPCMFrameCount, audio_data.samples);
	
	drwav_uninit(&wav_data);
	return audio_data;
}

bool unload_audio_file(audio_data_t *const audio_data_ptr) {
	free(audio_data_ptr->samples);
	audio_data_ptr->num_samples = 0;
	audio_data_ptr->samples = NULL;
}