#ifndef _RG_EVENT_HPP_
#define _RG_EVENT_HPP_

#include "core/basic.hpp"
#include "collections/farray.hpp"
#include "collections/darray.hpp"

namespace rg
{

// NOTE: first two events have same values as glfw.
// Don't reorder!
enum EventCode : uint
{
	MOUSE_DOWN,
	MOUSE_UP,
	MOUSE_MOVE,
	MOUSE_WHEEL,
	KEY_DOWN,
	KEY_UP,
	RESIZE,
	EnumSize
};

union EventContext
{
    Array<void*, 2> as_raw;
    Array<u64, 2> as_u64;
    Array<s64, 2> as_i64;
    Array<f64, 2> as_f64;
    Array<u32, 2> as_u32;
    Array<s32, 2> as_i32;
    Array<f32, 2> as_f32;
    Array<u16, 2> as_u16;
    Array<s16, 2> as_i16;
    Array<u8, 2> as_u8;
    Array<s8, 2> as_i8;
    Array<bool, 2> as_b8;

    EventContext() = default;
    EventContext(const EventContext& rhs)
    {
        rg::mem_copy(this, &rhs, sizeof(EventContext));
    }
    EventContext& operator=(const EventContext& rhs)
    {
        ASSERT(this != &rhs);
        rg::mem_copy(this, &rhs, sizeof(EventContext));
        return *this;
    }
};

// if returns true, dont proceed with calls to other events (event has been handled)
alias EventHandlerFn = bool(*)(EventContext ctx, void* listener);

struct Event
{
	EventHandlerFn handler;
	void* listener;
};

struct EventSystem
{
    static constexpr sz DEFAULT_CAPACITY = 256;
	EnumArray<DArray<Event>, EventCode> event_arrays;

	void init();
	void clear();
    void add_event(EventCode code, EventHandlerFn handler, void* listener);
    void fire_event(EventCode code, EventContext ev_ctx);
    void remove_event(EventCode code, EventHandlerFn handler);
};

} // rg

#endif // _RG_EVENT_HPP_ 
