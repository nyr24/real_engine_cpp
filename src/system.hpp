#ifndef _RG_SYSTEM_HPP_
#define _RG_SYSTEM_HPP_

#include "io.hpp"

namespace rg
{
    struct SystemInfo
    {
    	sz thread_count;
    	sz page_size;
    	Path cwd;
    };

    void get_system_info(Allocator* alloc, SystemInfo* sys_info);
} // rg

#endif // _RG_SYSTEM_HPP_
