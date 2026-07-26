#include "core/basic.hpp"
#include "engine/event.hpp"

namespace rg
{
 
void EventSystem::init()
{
 //    Context* ctx = get_context();
	// for (auto& event_arr : this->event_arrays)
	// {
	// 	event_arr.init_capacity(ctx->allocator, DEFAULT_CAPACITY);
	// }
}

void EventSystem::add_immediate_handler(EventCode code, EventHandlerFn handler, void* listener)
{
	this->immediate_events[code].push({ handler, listener });
}

void EventSystem::execute_immediate_handlers(EventCode code, EventContext ev_ctx)
{
	for (const ImmediateEvent& event : this->immediate_events[code])
	{
		if (event.handler(ev_ctx, event.listener))
		{
			break;
		}
	}
}

void EventSystem::remove_immediate_handler(EventCode code, EventHandlerFn handler)
{
    sz idx = 0;
	for (const ImmediateEvent& ev : this->immediate_events[code])
	{
		if (handler == ev.handler)
		{
			this->immediate_events[code].remove_unordered_at(idx);
		}
		++idx;
	}
}    

} // rg

