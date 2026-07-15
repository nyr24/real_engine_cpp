#include "engine/input.hpp"
#include "engine/entry.hpp"
#include "engine/event.hpp"

namespace rg
{

void InputSystem::init(Milliseconds rate, Milliseconds delay)
{
	rg::mem_zero(this, sizeof(*this));
	this->key_repeat_state.rate = rate;
	this->key_repeat_state.delay = delay;
}

void InputSystem::update()
{
    rg::mem_copy(&this->kb_prev, &this->kb_curr, sizeof(KeyboardState));
    rg::mem_copy(&this->mouse_prev, &this->mouse_curr, sizeof(MouseState));

	for (sz i = 0; i < GLFW_KEY_LAST; ++i)
	{
		if (this->kb_prev[i])
		{
			KeyRepeatTimer* timer = &this->key_repeat_timers[i];

			if (!timer->clock.is_running())
			{
				timer->clock.start();
				continue;
			}

			timer->clock.update();

			if (!timer->pass_delay)
			{
				if (timer->clock.progress > this->key_repeat_state.delay)
				{
					timer->pass_delay = true;
					timer->clock.restart();
				}
			}
			else if (timer->clock.progress > this->key_repeat_state.rate)
			{
				this->process_key(i, true);
				timer->clock.restart();
			}
		}
		else
		{
			KeyRepeatTimer* timer = &this->key_repeat_timers[i];
			timer->clock.stop();
			timer->pass_delay = false;
		}
	}
}

// ----- Keyboard input ------

void InputSystem::process_key(sz key, bool is_pressed)
{
	this->kb_curr[key] = is_pressed;
	EventContext ev_ctx;
	ev_ctx.as_u32[0] = key;

	EngineContext* engine_ctx = get_engine_context();
	engine_ctx->event_sys.fire_event(
		is_pressed ? KEY_DOWN : KEY_UP,
		ev_ctx
	);
}

// ----- Mouse input ------

void InputSystem::process_mouse_button(MouseButton button, bool is_pressed)
{
	this->mouse_curr.buttons[button] = is_pressed;
	EventContext ev_ctx;
	ev_ctx.as_u32[0] = (uint)button;

	EngineContext* engine_ctx = get_engine_context();
	engine_ctx->event_sys.fire_event(
		is_pressed ? MOUSE_DOWN : MOUSE_UP,
		ev_ctx
	);
}

void InputSystem::process_mouse_move(MousePos pos)
{
	[[unlikely]] if (!this->accept_move_events)
	{
		return;
	}

	static bool first_mouse_move = true;

	[[unlikely]] if (first_mouse_move)
	{
		this->mouse_curr.pos = pos;
		first_mouse_move = false;
		return;
	}

	this->mouse_curr.pos = pos;
	this->mouse_delta.x = this->mouse_curr.pos.x
		- this->mouse_prev.pos.x;
	this->mouse_delta.y = this->mouse_curr.pos.y
		- this->mouse_prev.pos.y;

	EventContext ev_ctx;
	ev_ctx.as_f32[0] = this->mouse_delta.x;
	ev_ctx.as_f32[1] = this->mouse_delta.y;

	EngineContext* engine_ctx = get_engine_context();
	engine_ctx->event_sys.fire_event(EventCode::MOUSE_MOVE, ev_ctx);
}

void InputSystem::process_mouse_wheel(bool z_delta)
{
	EventContext ev_ctx;
	ev_ctx.as_b8[0] = z_delta;
	EngineContext* engine_ctx = get_engine_context();
	engine_ctx->event_sys.fire_event(EventCode::MOUSE_WHEEL, ev_ctx);
}

bool operator==(const MousePos& first, const MousePos& sec)
{
	return first.x == sec.x && first.y == sec.y;
}

} // rg

