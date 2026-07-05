#ifndef _RG_IO_HPP_
#define _RG_IO_HPP_

#include "basic.hpp"
#include "slice.hpp"
#include "string.hpp"

namespace rg
{
#ifdef RG_PLATFORM_WIN32
    constexpr char PATH_SEPARATOR = '\\';
    constexpr char PATH_SEPARATOR_INVALID = '/';
    inline StrView RELATIVE_DIR_SELF = { CSTR_SIZED(".\\") };
    inline StrView RELATIVE_DIR_PARENT = { CSTR_SIZED("..\\") };
#else
    constexpr char PATH_SEPARATOR = '/';
    constexpr char PATH_SEPARATOR_INVALID = '\\';
    inline StrView RELATIVE_DIR_SELF = { CSTR_SIZED("./") };
    inline StrView RELATIVE_DIR_PARENT = { CSTR_SIZED("../") };
#endif

    constexpr sz PATH_INIT_CAPACITY = 128;
    constexpr sz FILE_MEMORY_MAP_BOUNDARY = 10 * MB;

    struct Path : DString
    {
        using DString::DString;
        using DString::init;
        void init(Allocator* alloc, Slice<char> input_path, bool null_term = true);
    private:
        void init_from_cwd(Allocator* alloc, sz steps_back = 0, sz add_cap = 0);
    #ifdef RG_PLATFORM_WIN32
    public:
        bool is_utf16;
        void ensure_utf16();
    private:
        void convert_to_utf16();
    #endif
    };

    sz get_steps_back_from_cwd_and_trim(Slice<char>* path);
    inline bool path_is_absolute(Slice<char> path);

    struct MemoryMapEntry
    {
    #ifdef RG_PLATFORM_WIN32
        FileHandle file_handle;
        FileHandle file_mapping;
    #else
        sz size;
        u8* mapped_mem;
    #endif

        void unmap();
        inline Slice<u8> slice() { return { this->mapped_mem, size }; }
    };

    enum struct FileOpenBit : u32
    {
        READ        = 1 << 0,
        WRITE       = 1 << 1,
        APPEND      = 1 << 2,
        READ_WRITE  = READ | WRITE,
        READ_APPEND = READ | APPEND,
    };

    enum struct FileSeekPos : s32
    {
    #ifdef RG_PLATFORM_POSIX
        START = SEEK_SET,
        CUR   = SEEK_CUR,
        END   = SEEK_END,
    #else
        START = FILE_BEGIN,
        CUR   = FILE_CURRENT,
        END   = FILE_END,
    #endif
    };

    inline s32 operator|(FileOpenBit a, FileOpenBit b) { return s32(u32(a) | u32(b)); }
    inline s32 operator&(FileOpenBit a, FileOpenBit b) { return s32(u32(a) & u32(b)); }
    inline s32 operator|=(FileOpenBit& a, FileOpenBit b) { return s32(u32(a) & u32(b)); }

    bool file_open(Path* path, FileHandle* out_handle, FileOpenBit open_flags);
    bool file_close(FileHandle handle);
    sz file_get_size(FileHandle handle);
    sz file_get_size_from_path(Path* path);
    bool file_read(Allocator* alloc, Path* path, DString* out_data);
    bool file_write(Path* path, Slice<u8> data, bool at_end = true);
    bool file_memory_map(Path* path, MemoryMapEntry* out_entry, sz file_size = 0);
    bool file_set_cursor(FileHandle handle, FileSeekPos mode, sz offset = 0);
    bool file_get_cursor(FileHandle handle, sz* out_cursor);
    bool memory_map(MemoryMapEntry* out_entry);

} // rg

#endif // _RG_IO_HPP_
