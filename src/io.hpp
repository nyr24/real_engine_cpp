#ifndef _RG_IO_HPP_
#define _RG_IO_HPP_

#include "basic.hpp"
#include "string.hpp"

namespace rg
{
#ifdef RG_PLATFORM_WIN32
    const char PATH_SEPARATOR = '\\';
    const char PATH_SEPARATOR_INVALID = '/';
#else
    const char PATH_SEPARATOR = '/';
    const char PATH_SEPARATOR_INVALID = '\\';
#endif

    struct Path : DString
    {
        void init_absolute(Allocator* alloc, StrView path, bool null_term = true);
        void init_absolute(Allocator* alloc, CString path, bool null_term = true);
    private:
        // NOTE: private?
        void init_from_cwd(Allocator* alloc, sz add_cap = 0);
    };

    struct File
    {
        FileHandle handle;

        sz len();
        bool close();
    };

    File file_open();
    File file_from_handle(FileHandle handle);
    DString file_read_entire(Allocator* alloc, Path path);
    StrView file_read_entire_into(Path path, StrView buff);
    void file_write(Path path, StrView write_buff);

} // rg

#endif // _RG_IO_HPP_
