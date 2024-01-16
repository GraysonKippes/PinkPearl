#include "camera.h"

#include <stdint.h>

#include "util/time.h"

static vector3F_t camera_position = { 0 };
static vector3F_t old_camera_position = { 0 };
static vector3F_t new_camera_position = { 0 };

static bool camera_scrolling = false;

static uint64_t time_ms_since_scroll_start = 0;
