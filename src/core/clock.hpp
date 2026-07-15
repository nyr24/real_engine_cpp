#ifndef _RG_CLOCK_HPP_
#define _RG_CLOCK_HPP_

#include "core/basic.hpp"

// Clock.

namespace rg
{

struct Clock
{
    Nanoseconds start_time;
    Nanoseconds progress;

    void start();
    Nanoseconds update_and_get_delta();
    void update();
    void stop();
    void restart();
    bool is_running() { return this->start_time > 0; }
};

} // rg

#endif // _RG_CLOCK_HPP_
