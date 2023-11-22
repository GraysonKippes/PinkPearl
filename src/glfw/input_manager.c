#include "input_manager.h"

#define NUM_INPUTS (GLFW_KEY_LAST + GLFW_MOUSE_BUTTON_LAST)

static const int num_key_inputs = GLFW_KEY_LAST;
static const int num_mouse_inputs = GLFW_MOUSE_BUTTON_LAST;
static const int num_inputs = NUM_INPUTS;

static input_state_t input_states[NUM_INPUTS];

input_state_t get_input_state(int input_index) {
	return input_states[input_index];
}

bool is_input_pressed_or_held(int input_index) {
	return input_states[input_index] != INPUT_STATE_RELEASED;
}

static void update_input_state(int input_index, int action) {

	if (action == GLFW_RELEASE) {
		input_states[input_index] = INPUT_STATE_RELEASED;
	}
	else if (input_states[input_index] == INPUT_STATE_RELEASED) {
		input_states[input_index] = INPUT_STATE_HELD;
	}
	else {
		input_states[input_index] = INPUT_STATE_PRESSED;
	}
}

static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {

	if (key == GLFW_KEY_UNKNOWN) {
		return;
	}

	update_input_state(key, action);
}

static void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {

	update_input_state(button + num_key_inputs, action);
}

void init_input_manager(GLFWwindow *window) {
	glfwSetKeyCallback(window, glfw_key_callback);
	glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
}
