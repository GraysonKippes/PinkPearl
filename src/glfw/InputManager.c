#include "InputManager.h"

typedef enum InputState {
	INPUT_STATE_PRESSED = 0,
	INPUT_STATE_HELD = 1,
	INPUT_STATE_RELEASED = 2
} InputState;

static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

#define MAX_INPUT_COUNT (GLFW_KEY_LAST + GLFW_MOUSE_BUTTON_LAST)
static const int keyInputCount = GLFW_KEY_LAST;
static const int maxInputCount = MAX_INPUT_COUNT;
static InputState inputStates[MAX_INPUT_COUNT] = { };

void initInputManager(GLFWwindow *window) {
	for (int i = 0; i < maxInputCount; ++i) {
		inputStates[i] = INPUT_STATE_RELEASED;
	}
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
}

bool isInputPressed(const int input) {
	if (inputStates[input] == INPUT_STATE_PRESSED) {
		inputStates[input] = INPUT_STATE_HELD;
		return true;
	}
	return false;
}

bool isInputPressedOrHeld(const int input) {
	bool pressedOrHeld = inputStates[input] != INPUT_STATE_RELEASED;
	if (pressedOrHeld) {
		inputStates[input] = INPUT_STATE_HELD;
	}
	return pressedOrHeld;
}

static void updateInputState(const int input, const int action) {
	if (action == GLFW_RELEASE) {
		inputStates[input] = INPUT_STATE_RELEASED;
	} else if (action == GLFW_PRESS) {
		inputStates[input] = INPUT_STATE_PRESSED;
	}
}

static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	(void)window;
	(void)scancode;
	(void)mods;
	if (key == GLFW_KEY_UNKNOWN) {
		return;
	}
	updateInputState(key, action);
}

static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
	(void)window;
	(void)mods;
	updateInputState(button + keyInputCount, action);
}

char *getInputName(const int input) {
	switch (input) {
		case 0: return "None";
		default: return "Unknown";
		case GLFW_KEY_SPACE: return "Space";
		case GLFW_KEY_APOSTROPHE: return "'";
		case GLFW_KEY_COMMA: return ",";
		case GLFW_KEY_MINUS: return "-";
		case GLFW_KEY_PERIOD: return ".";
		case GLFW_KEY_SLASH: return "/";
		case GLFW_KEY_0: return "0";
		case GLFW_KEY_1: return "1";
		case GLFW_KEY_2: return "2";
		case GLFW_KEY_3: return "3";
		case GLFW_KEY_4: return "4";
		case GLFW_KEY_5: return "5";
		case GLFW_KEY_6: return "6";
		case GLFW_KEY_7: return "7";
		case GLFW_KEY_8: return "8";
		case GLFW_KEY_9: return "9";
		case GLFW_KEY_SEMICOLON: return ";";
		case GLFW_KEY_EQUAL: return "=";
		case GLFW_KEY_A: return "A";
		case GLFW_KEY_B: return "B";
		case GLFW_KEY_C: return "C";
		case GLFW_KEY_D: return "D";
		case GLFW_KEY_E: return "E";
		case GLFW_KEY_F: return "F";
		case GLFW_KEY_G: return "G";
		case GLFW_KEY_H: return "H";
		case GLFW_KEY_I: return "I";
		case GLFW_KEY_J: return "J";
		case GLFW_KEY_K: return "K";
		case GLFW_KEY_L: return "L";
		case GLFW_KEY_M: return "M";
		case GLFW_KEY_N: return "N";
		case GLFW_KEY_O: return "O";
		case GLFW_KEY_P: return "P";
		case GLFW_KEY_Q: return "Q";
		case GLFW_KEY_R: return "R";
		case GLFW_KEY_S: return "S";
		case GLFW_KEY_T: return "T";
		case GLFW_KEY_U: return "U";
		case GLFW_KEY_V: return "V";
		case GLFW_KEY_W: return "W";
		case GLFW_KEY_X: return "X";
		case GLFW_KEY_Y: return "Y";
		case GLFW_KEY_Z: return "Z";
		case GLFW_KEY_LEFT_BRACKET: return "[";
		case GLFW_KEY_BACKSLASH: return "\\";
		case GLFW_KEY_RIGHT_BRACKET: return "]";
		case GLFW_KEY_GRAVE_ACCENT: return "`";
		case GLFW_KEY_ESCAPE: return "Escape";
		case GLFW_KEY_ENTER: return "Enter";
		case GLFW_KEY_TAB: return "Tab";
		case GLFW_KEY_BACKSPACE: return "Backspace";
		case GLFW_KEY_INSERT: return "Insert";
		case GLFW_KEY_DELETE: return "Delete";
		case GLFW_KEY_RIGHT: return "Right";
		case GLFW_KEY_LEFT: return "Left";
		case GLFW_KEY_DOWN: return "Down";
		case GLFW_KEY_UP: return "Up";
		case GLFW_KEY_PAGE_UP: return "Page Up";
		case GLFW_KEY_PAGE_DOWN: return "Page Down";
		case GLFW_KEY_HOME: return "Home";
		case GLFW_KEY_END: return "End";
		case GLFW_KEY_CAPS_LOCK: return "Caps Lock";
		case GLFW_KEY_SCROLL_LOCK: return "Scroll Lock";
		case GLFW_KEY_NUM_LOCK: return "Num Lock";
		case GLFW_KEY_PRINT_SCREEN: return "Print Screen";
		case GLFW_KEY_PAUSE: return "Pause";
		case GLFW_KEY_F1: return "F1";
		case GLFW_KEY_F2: return "F2";
		case GLFW_KEY_F3: return "F3";
		case GLFW_KEY_F4: return "F4";
		case GLFW_KEY_F5: return "F5";
		case GLFW_KEY_F6: return "F6";
		case GLFW_KEY_F7: return "F7";
		case GLFW_KEY_F8: return "F8";
		case GLFW_KEY_F9: return "F9";
		case GLFW_KEY_F10: return "F10";
		case GLFW_KEY_F11: return "F11";
		case GLFW_KEY_F12: return "F12";
		case GLFW_KEY_F13: return "F13";
		case GLFW_KEY_F14: return "F14";
		case GLFW_KEY_F15: return "F15";
		case GLFW_KEY_F16: return "F16";
		case GLFW_KEY_F17: return "F17";
		case GLFW_KEY_F18: return "F18";
		case GLFW_KEY_F19: return "F19";
		case GLFW_KEY_F20: return "F20";
		case GLFW_KEY_F21: return "F21";
		case GLFW_KEY_F22: return "F22";
		case GLFW_KEY_F23: return "F23";
		case GLFW_KEY_F24: return "F24";
		case GLFW_KEY_F25: return "F25";
		case GLFW_KEY_KP_0: return "Keypad 0";
		case GLFW_KEY_KP_1: return "Keypad 1";
		case GLFW_KEY_KP_2: return "Keypad 2";
		case GLFW_KEY_KP_3: return "Keypad 3";
		case GLFW_KEY_KP_4: return "Keypad 4";
		case GLFW_KEY_KP_5: return "Keypad 5";
		case GLFW_KEY_KP_6: return "Keypad 6";
		case GLFW_KEY_KP_7: return "Keypad 7";
		case GLFW_KEY_KP_8: return "Keypad 8";
		case GLFW_KEY_KP_9: return "Keypad 9";
		case GLFW_KEY_KP_DECIMAL: return "Keypad Decimal";
		case GLFW_KEY_KP_DIVIDE: return "Keypad Divide";
		case GLFW_KEY_KP_MULTIPLY: return "Keypad Multiply";
		case GLFW_KEY_KP_SUBTRACT: return "Keypad Subtract";
		case GLFW_KEY_KP_ADD: return "Keypad Add";
		case GLFW_KEY_KP_ENTER: return "Keypad Enter";
		case GLFW_KEY_KP_EQUAL: return "Keypad Equal";
		case GLFW_KEY_LEFT_SHIFT: return "Left Shift";
		case GLFW_KEY_LEFT_CONTROL: return "Left Control";
		case GLFW_KEY_LEFT_ALT: return "Left Alt";
		case GLFW_KEY_LEFT_SUPER: return "Left Super";
		case GLFW_KEY_RIGHT_SHIFT: return "Right Shift";
		case GLFW_KEY_RIGHT_CONTROL: return "Right Control";
		case GLFW_KEY_RIGHT_ALT: return "Right Alt";
		case GLFW_KEY_RIGHT_SUPER: return "Right Super";
		case MOUSE_BUTTON_LEFT: return "Mouse Left";
		case MOUSE_BUTTON_MIDDLE: return "Mouse Middle";
		case MOUSE_BUTTON_RIGHT: return "Mouse Right";
		case MOUSE_BUTTON_4: return "Mouse 4";
		case MOUSE_BUTTON_5: return "Mouse 5";
		case MOUSE_BUTTON_6: return "Mouse 6";
		case MOUSE_BUTTON_7: return "Mouse 7";
		case MOUSE_BUTTON_8: return "Mouse 8";
	}
}
