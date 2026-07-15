#ifndef _RG_INPUT_HPP_
#define _RG_INPUT_HPP_

#include "core/basic.hpp"
#include "core/clock.hpp"
#include "collections/farray.hpp"
#include "GLFW/glfw3.h"

namespace rg
{

struct MousePos
{
	f32 x;
	f32 y;
};

bool operator==(const MousePos& first, const MousePos& sec);

// down = true, up = false.
alias KeyboardState = Array<bool, GLFW_KEY_LAST>;
alias MouseDelta = MousePos;

enum struct MouseButton
{
	LEFT   = GLFW_MOUSE_BUTTON_LEFT,
	RIGHT  = GLFW_MOUSE_BUTTON_RIGHT,
	MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE,
	EnumSize
};

struct MouseState
{
	MousePos pos;
	// down = true, up = false
	EnumArray<bool, MouseButton> buttons;
};

struct KeyRepeatGlobalState
{
	Nanoseconds rate;
	Nanoseconds delay;
};

struct KeyRepeatTimer
{
	Clock clock;
	bool pass_delay;
};

struct InputSystem
{
    static sz constexpr DEFAULT_KEY_REPEAT_RATE_NS = 10 * MILLI_SEC;
    static sz constexpr DEFAULT_KEY_REPEAT_DELAY_MS = 100 * MILLI_SEC;

	MouseState mouse_curr;
	MouseState mouse_prev;
	KeyboardState kb_curr;
	KeyboardState kb_prev;
	MouseDelta mouse_delta;
	KeyRepeatGlobalState key_repeat_state;
	Array<KeyRepeatTimer, GLFW_KEY_LAST> key_repeat_timers;
	bool accept_move_events;

	void init(Milliseconds rate = DEFAULT_KEY_REPEAT_RATE_NS, Milliseconds delay = DEFAULT_KEY_REPEAT_DELAY_MS);
	void update();

    bool is_key_down(sz key) { return this->kb_curr[key]; }
    bool is_key_up(sz key) { return !this->kb_curr[key]; }
    bool was_key_down(sz key) { return this->kb_prev[key]; }
    bool was_key_up(sz key) { return !this->kb_prev[key]; }
    bool is_button_down(MouseButton button) { return this->mouse_curr.buttons[button]; };
    bool is_button_up(MouseButton button) { return !this->mouse_curr.buttons[button]; }
    bool was_button_down(MouseButton button) { return this->mouse_prev.buttons[button]; }
    bool was_button_up(MouseButton button) { return !this->mouse_prev.buttons[button]; }
    MousePos get_mouse_position() { return this->mouse_curr.pos; }
    MousePos get_previous_mouse_position() { return this->mouse_prev.pos; }
    void process_key(sz key, bool is_pressed);
    void process_mouse_button(MouseButton button, bool is_pressed);
    void process_mouse_move(MousePos pos);
    void process_mouse_wheel(bool z_delta);
};

} // rg

#endif // _RG_WINDOW_HPP_
