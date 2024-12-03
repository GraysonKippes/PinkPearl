#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <GLFW/glfw3.h>

// These mouse button defines offset the GLFW mouse buttons by the number of keys, effectively differentiating mouse buttons from keys.

#define MOUSE_BUTTON_LEFT (GLFW_KEY_LAST + GLFW_MOUSE_BUTTON_LEFT)
#define MOUSE_BUTTON_MIDDLE (GLFW_KEY_LAST + GLFW_MOUSE_BUTTON_MIDDLE)
#define MOUSE_BUTTON_RIGHT (GLFW_KEY_LAST + GLFW_MOUSE_BUTTON_RIGHT)
#define MOUSE_BUTTON_1 (GLFW_KEY_LAST + GLFW_MOUSE_BUTTON_1)
#define MOUSE_BUTTON_2 (GLFW_KEY_LAST + GLFW_MOUSE_BUTTON_2)
#define MOUSE_BUTTON_3 (GLFW_KEY_LAST + GLFW_MOUSE_BUTTON_3)
#define MOUSE_BUTTON_4 (GLFW_KEY_LAST + GLFW_MOUSE_BUTTON_4)
#define MOUSE_BUTTON_5 (GLFW_KEY_LAST + GLFW_MOUSE_BUTTON_5)
#define MOUSE_BUTTON_6 (GLFW_KEY_LAST + GLFW_MOUSE_BUTTON_6)
#define MOUSE_BUTTON_7 (GLFW_KEY_LAST + GLFW_MOUSE_BUTTON_7)
#define MOUSE_BUTTON_8 (GLFW_KEY_LAST + GLFW_MOUSE_BUTTON_8)

// Initializes the input manager. Application input will not work until this is called.
// Do not call this until after GLFW is initialized.
void initInputManager(GLFWwindow *window);

// Returns true if the specified input was just pressed, false otherwise.
// If an input is pressed, getting the input state "consumes" it, setting the state to held.
bool isInputPressed(const int input);

// Returns true if the specified input is being pressed or held at all.
// If an input is pressed, getting the input state "consumes" it, setting the state to held.
bool isInputPressedOrHeld(const int input);

// Returns a human-readable name for an input, or "Unknown" for an unknown input code.
char *getInputName(const int input);

#endif	// INPUT_MANAGER_H
