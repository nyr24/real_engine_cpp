#ifndef _RG_CONTEXT_HPP_
#define _RG_CONTEXT_HPP_

#include "core/thread.hpp"

namespace rg
{

struct AppContext
{
    Mutex logger_mutex;

    static AppContext& get()
    {
        persist AppContext app_ctx;
        return app_ctx;
    }
    void init();
    void destroy();
};

} // rg

#endif // _RG_CONTEXT_HPP_


