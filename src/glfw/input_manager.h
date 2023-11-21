#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <GLFW/glfw3.h>

typedef enum input_state_t {
	INPUT_STATE_PRESSED = 0,
	INPUT_STATE_HELD = 1,
	INPUT_STATE_RELEASED = 2
} input_state_t;

void init_input_manager(GLFWwindow *window);

input_state_t get_input_state(int input_index);

#endif	// INPUT_MANAGER_H
