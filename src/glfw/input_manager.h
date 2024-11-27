#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

// TODO: implement both synchronous and asynchronous input management.
// Synchronous: input is pushed to a queue whence it is consumed on subsequent game ticks.
// Asynchronous: input is handled directly in the callback (or functions called by the callback).

#include <stdbool.h>
#include <GLFW/glfw3.h>

typedef enum input_state_t {
	INPUT_STATE_PRESSED = 0,
	INPUT_STATE_HELD = 1,
	INPUT_STATE_RELEASED = 2
} input_state_t;

void init_input_manager(GLFWwindow *window);

input_state_t get_input_state(int input_index);

bool is_input_pressed_or_held(int input_index);

bool isInputPressed(const int inputIndex);

#endif	// INPUT_MANAGER_H
