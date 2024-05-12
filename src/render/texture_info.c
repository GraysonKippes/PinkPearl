#include "texture_info.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log/error_code.h"
#include "log/log_stack.h"
#include "log/logging.h"
#include "util/allocate.h"
#include "util/file_io.h"

bool is_animation_set_empty(texture_create_info_t animation_set) {
	return animation_set.num_animations == 0 || animation_set.animations == NULL;
}

void destroy_texture_pack(texture_pack_t *texture_pack_ptr) {

	if (texture_pack_ptr == NULL) {
		return;
	}

	for (uint32_t i = 0; i < texture_pack_ptr->num_textures; ++i) {

		free(texture_pack_ptr->texture_create_infos[i].path);
		texture_pack_ptr->texture_create_infos[i].path = NULL;

		free(texture_pack_ptr->texture_create_infos[i].animations);
		texture_pack_ptr->texture_create_infos[i].animations = NULL;
	}

	free(texture_pack_ptr->texture_create_infos);
	texture_pack_ptr->texture_create_infos = NULL;
}

texture_pack_t parse_fgt_file(const char *path) {

	log_stack_push("LoadTexturePack");
	logf_message(VERBOSE, "Loading texture pack from \"%s\"...", path);

	texture_pack_t texture_pack = { 0 };
	texture_pack.num_textures = 0;
	texture_pack.texture_create_infos = NULL;

	if (path == NULL) {
		log_message(ERROR, "Filename is NULL.");
		return texture_pack;
	}

	FILE *fgt_file = fopen(path, "rb");
	if (fgt_file == NULL) {
		logf_message(ERROR, "File not found at \"%s\".", path);
		return texture_pack;
	}

	static const char fgt_label[4] = "FGT";
	char label[4];
	fread(label, 1, 4, fgt_file);
	if (strcmp(label, fgt_label) != 0) {
		logf_message(ERROR, "Invalid file format; found label \"%s\".", label);
		goto end_read;
	}

	read_data(fgt_file, sizeof(uint32_t), 1, &texture_pack.num_textures);

	if (texture_pack.num_textures == 0) {
		log_message(ERROR, "Number of textures specified as zero.");
		goto end_read;
	}

	if (!allocate((void **)&texture_pack.texture_create_infos, texture_pack.num_textures, sizeof(texture_create_info_t))) {
		log_message(ERROR, "Texture create info array allocation failed.");
		goto end_read;
	}

	for (uint32_t i = 0; i < texture_pack.num_textures; ++i) {

		static const size_t max_path_len = 64;

		texture_create_info_t *info_ptr = texture_pack.texture_create_infos + i;

		if (!allocate((void **)&info_ptr->path, max_path_len, sizeof(char))) {
			log_message(ERROR, "Texture create info path allocation failed.");
			goto end_read;
		}

		const int path_read_result = read_string(fgt_file, max_path_len, info_ptr->path);
		if (path_read_result < 0) {
			logf_message(ERROR, "Texture create info path reading failed. (Error code: %i)", path_read_result);
			goto end_read;
		}
		else if (path_read_result == 1) {
			log_message(WARNING, "Texture create info path reached max length before finding null-terminator; the data is likely not formatted well.");
		}

		// Read texture type.
		read_data(fgt_file, sizeof(uint32_t), 1, &info_ptr->type);

		// Read number of cells in texture atlas.
		read_data(fgt_file, sizeof(uint32_t), 1, &info_ptr->num_cells.width);
		read_data(fgt_file, sizeof(uint32_t), 1, &info_ptr->num_cells.length);

		// Read texture cell extent.
		read_data(fgt_file, sizeof(uint32_t), 1, &info_ptr->cell_extent.width);
		read_data(fgt_file, sizeof(uint32_t), 1, &info_ptr->cell_extent.length);

		// Check extents -- if any of them are zero, then there certainly was an error.
		if (info_ptr->num_cells.width == 0) {
			log_message(WARNING, "Texture create info number of cells widthwise is zero.");
		}
		if (info_ptr->num_cells.length == 0) {
			log_message(WARNING, "Texture create info number of cells lengthwise is zero.");
		}
		if (info_ptr->cell_extent.width == 0) {
			log_message(WARNING, "Texture create info cell extent width is zero.");
		}
		if (info_ptr->cell_extent.length == 0) {
			log_message(WARNING, "Texture create info cell extent length is zero.");
		}
		
		// Calculate texture extent.
		info_ptr->atlas_extent.width = info_ptr->num_cells.width * info_ptr->cell_extent.width;
		info_ptr->atlas_extent.length = info_ptr->num_cells.length * info_ptr->cell_extent.length;

		// Read animation create infos.
		read_data(fgt_file, sizeof(uint32_t), 1, &info_ptr->num_animations);
		if (info_ptr->num_animations > 0) {

			if (!allocate((void **)&info_ptr->animations, info_ptr->num_animations, sizeof(animation_create_info_t))) {
				logf_message(ERROR, "Failed to allocate array of animation create infos in texture %u.", i);
				goto end_read;
			}

			for (uint32_t j = 0; j < info_ptr->num_animations; ++j) {
				read_data(fgt_file, sizeof(uint32_t), 1, &info_ptr->animations[j].start_cell);
				read_data(fgt_file, sizeof(uint32_t), 1, &info_ptr->animations[j].num_frames);
				read_data(fgt_file, sizeof(uint32_t), 1, &info_ptr->animations[j].frames_per_second);
			}
		}
		else {

			// If there are no specified animations, then set the number of animations to one 
			// 	and set the first (and only) animation to a default value.
			// This eliminates the need for branching when querying animation cycles in a texture.
			info_ptr->num_animations = 1;
			if (!allocate((void **)&info_ptr->animations, info_ptr->num_animations, sizeof(animation_create_info_t))) {
				logf_message(ERROR, "Error reading texture file: failed to allocate array of animation create infos in texture %u.", i);
				goto end_read;
			}

			info_ptr->animations[0].start_cell = 0;
			info_ptr->animations[0].num_frames = 1;
			info_ptr->animations[0].frames_per_second = 0;
		}
	}

end_read:
	fclose(fgt_file);
	error_queue_flush();
	log_stack_pop();
	return texture_pack;
}
