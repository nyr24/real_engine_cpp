#include "core/basic.hpp"
#include "engine/entry.hpp"
#include "engine/event.hpp"

namespace rg
{
 
void EventSystem::init()
{
    Context* ctx = get_context();
	for (auto& event_arr : this->event_arrays)
	{
		event_arr.init_capacity(ctx->allocator, DEFAULT_CAPACITY);
	}
}

void EventSystem::clear()
{
	for (auto& event_arr : this->event_arrays)
	{
		event_arr.clear();
	}
}

void EventSystem::add_event(EventCode code, EventHandlerFn handler, void* listener)
{
	this->event_arrays[code].push(Event{ handler, listener });
}

void EventSystem::fire_event(EventCode code, EventContext ev_ctx)
{
	for (const Event& event : this->event_arrays[code])
	{
		if (event.handler(ev_ctx, event.listener))
		{
			break;
		}
	}
}

void EventSystem::remove_event(EventCode code, EventHandlerFn handler)
{
    sz idx = 0;
	for (const Event& ev : this->event_arrays[code])
	{
		if (handler == ev.handler)
		{
			this->event_arrays[code].remove_unordered_at(idx);
		}
		++idx;
	}
}    

} // rg

