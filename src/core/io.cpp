#include "core/basic.hpp"
#include "core/io.hpp"
#include "collections/slice.hpp"

namespace rg
{

// Path.

intern void platform_process_cwd(Path* path, Allocator* alloc, sz add_cap = 0);
intern sz get_steps_back_from_cwd_and_trim(Slice<char>* path);

void Path::init(Allocator* alloc, Slice<char> path, bool null_term)
{
    path.replace(PATH_SEPARATOR_INVALID, PATH_SEPARATOR);
    this->alloc = alloc;

    if (path_is_absolute(path))
    {
        this->init(alloc, path.count);
        this->push(path);
        if (null_term) this->ensure_null_term();
    }
    else
    {
        sz steps_back_from_cwd = get_steps_back_from_cwd_and_trim(&path);
        sz add_cap = path.count;
        platform_process_cwd(this, alloc, add_cap);
        // Do back steps.
        for (; steps_back_from_cwd > 0; --steps_back_from_cwd)
        {
            this->trim_from_end_to_last_occur(PATH_SEPARATOR);
        }
        // Add a path separator to the end.
        this->push(PATH_SEPARATOR);
        ASSERT_MSG(this->last() == PATH_SEPARATOR, "Must be separated at end");
        this->ensure_no_null_term();
        this->push(path);
        if (null_term) this->ensure_null_term();
    }
}

Path get_cwd(Allocator* alloc)
{
    Path path;
    platform_process_cwd(&path, alloc);
    return path;
}

intern void platform_process_cwd(Path* path, Allocator* alloc, sz add_cap)
{
    #ifdef RG_PLATFORM_WIN32
        path->capacity = add_cap + 1;
        u32 size = ::GetCurrentDirectory(0, null);
        path->capacity += (sz)size;
        path->data = (char*)allocator_allocate(alloc, path->capacity);
        path->count = ::GetCurrentDirectory(path->count, path->ptr);
    #else
        path->capacity = PATH_DEFAULT_CAPACITY + add_cap;
        path->data = (char*)allocator_allocate(alloc, path->capacity);
        while (true)
        {
            path->data = ::getcwd(path->data, path->capacity);
            // Success.
            if (path->data) break;
            // Failure - grow the buffer.
            path->capacity *= 2;
            path->data = (char*)allocator_reallocate(alloc, path->data, path->capacity);
        };
        path->count = path->capacity;
        path->trim_from_end_to_first_occur('\0', false);
        // How much space left in the allocated buffer.
        sz remain = path->capacity - path->count;
        if (remain < add_cap)
        {
            sz diff = add_cap - remain;
            path->capacity += diff;
            path->data = (char*)allocator_reallocate(alloc, path->data, path->capacity);
        }
    #endif
}

// Returns a number of steps it needs to take from the cwd.
// . == 0, ./ == 0, ../ == 1, ../../ == 2, etc.
// if it doesn't start with a dot - returns 0
intern sz get_steps_back_from_cwd_and_trim(Slice<char>* input_path)
{
    if (path_is_absolute(*input_path)) return 0;
    if (input_path->first() != '.') return 0;

    if (common_starts_with(input_path->ptr, input_path->count, RELATIVE_DIR_SELF.to_char_slice_unsafe()))
    {
        input_path->trim_start_n(2);
        return 0;
    }

    sz steps = 0;
    if (common_starts_with(input_path->ptr, input_path->count, RELATIVE_DIR_PARENT.to_char_slice_unsafe()))
    {
        input_path->trim_start_n(3);
        ++steps;
    }

    ASSERT_MSG(input_path->first() != PATH_SEPARATOR, "Path separators should be trimmed");
    return steps;
}

inline bool path_is_absolute(Slice<char> path)
{
    return path.first() == PATH_SEPARATOR;
}

#ifdef RG_PLATFORM_WIN32
void Path::ensure_utf16()
{
    if (this->is_utf16) return;
    this->convert_to_utf16();
}

void Path::convert_to_utf16()
{
    ASSERT_INITIALIZED(this);

    s32 wide_len = MultiByteToWideChar(CP_UTF8, 0, this->data, this->count, null, 0);
    if (wide_len == 0) return;

    sz new_size = (sizeof(wchar_t)) * wide_len;
    if (new_size > this->capacity)
    {
        this->capacity = new_size;
        this->data = (char*)allocator_reallocate(alloc, this->data, this->capacity);
        this->count = new_size;
    }

    ASSERT_EQ_ZERO(MultiByteToWideChar(CP_UTF8, 0, this->data, this->count, this->data, wide_len));
    this->is_utf16 = true;
}
#endif // win32

// Files.

intern bool file_read_default(Path* path, DString* out_data, sz precomputed_file_size = 0);
intern bool file_read_mmap(Path* path, DString* out_data, sz precomputed_file_size = 0);
intern bool file_write_default(Path* path, Slice<u8> data, bool at_end = true);
intern bool file_write_mmap(Path* path, Slice<u8> data, bool at_end = true);

bool file_open(Path* path, FileHandle* out_handle, FileOpenBit flags)
{
    ASSERT_INITIALIZED(path);
    path->ensure_null_term();

#ifdef RG_PLATFORM_WIN32
    DWORD windows_flags = 0;
    if (flags & FileOpenBit::READ)    windows_flags |= GENERIC_READ;
    if (flags & FileOpenBit::WRITE)   windows_flags |= GENERIC_WRITE;

    if (contains_non_ascii(path->data, path->end()))
    {
        path->convert_to_utf16();
        *out_handle = ::CreateFileW(path->data,
            windows_flags,
            GENERIC_READ | GENERIC_WRITE,
            null,
            OPEN_EXISTING,
            null,
            null);
    }
    else
    {
        *out_handle = ::CreateFileA(file_path,
            windows_flags,
            GENERIC_READ | GENERIC_WRITE,
            null,
            OPEN_EXISTING,
            null,
            null);
    }
#else
    u32 unix_flags = O_RDONLY;
    if (u32(flags & FileOpenBit::WRITE))
    {
        if (u32(flags & FileOpenBit::READ)) unix_flags = O_RDWR;
        else unix_flags = O_WRONLY;
    }
    *out_handle = ::open(path->cstr(), (s32)unix_flags);
#endif
    if (*out_handle == FILE_HANDLE_INVALID)
    {
        LOG_ERROR("Could not open file from path: " FMT_STR_LEN, GET_FMT_STR(path));
        return false;
    }
    return true;
}

bool file_close(FileHandle handle)
{
    if (handle == FILE_HANDLE_INVALID)
    {
        LOG_WARN("Trying to close the file that maybe already closed or wasn't opened");
        return false;
    }
    #ifdef RG_PLATFORM_WIN32
        if (!::CloseHandle(handle)) return false;
    #else
        if (handle <= 2) return false;  // Don't close stdin(0), stdout(1), stderr(2)
        if (::close(handle) != 0) return false;
    #endif // RG_PLATFORM_WIN32
    handle = FILE_HANDLE_INVALID;
    return true;
}

sz file_get_size(FileHandle handle)
{
    ASSERT_MSG(handle != FILE_HANDLE_INVALID, "Must be valid file handle");
#ifdef RG_PLATFORM_WIN32
    LARGE_INTEGER file_size;
    ASSERT_NON_ZERO(::GetFileSizeEx(handle, &file_size));
    return file_size.QuadPart;
#else
    struct stat stat_buf;
    ASSERT_EQ_ZERO(::fstat(handle, &stat_buf));
    return stat_buf.st_size;
#endif
}

sz file_get_size_from_path(Path* path)
{
    path->ensure_null_term();
#ifdef RG_PLATFORM_WIN32
    WIN32_FILE_ATTRIBUTE_DATA file_info;
    ASSERT_NON_ZERO(::GetFileAttributesEx(path->data, GetFileExInfoStandard, &file_info));
    sz file_size = sz((u64(file_info.nFileSizeHigh) << 32) | file_info.nFileSizeLow);
    return file_size;
#else
    struct stat stat_buf;
    ASSERT_EQ_ZERO(::stat(path->data, &stat_buf));
    return stat_buf.st_size;
#endif
}

bool file_memory_map(Path* path, FileMapEntry* out_entry, sz file_size)
{
#ifdef RG_PLATFORM_WIN32
    path->ensure_utf16();

    if (!file_open(path, &out_entry->file_handle, FileOpenBit::READ_WRITE)) return false;

    if (file_size == 0) file_size = file_get_size(out_entry->file_handle);
    out_entry->size = file_size;

    out_entry->file_mapping = ::CreateFileMapping(
        out_entry->file_handle, 
        0, 
        PAGE_READWRITE, 
        0, 0,
        0
    );

    out_entry->mapped_mem = (u8*)::MapViewOfFile(
        out_entry->file_mapping, 
        FILE_MAP_ALL_ACCESS, 
        0, 0,
        0
    );

    return true;
#else
    FileHandle fhandle;
    if (!file_open(path, &fhandle, FileOpenBit::READ_WRITE)) return false;
    defer(file_close(fhandle));

    if (file_size == 0) file_size = file_get_size(fhandle);
    out_entry->size = file_size;
    out_entry->mapped_mem = (u8*)::mmap(null, out_entry->size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fhandle, 0);
    ASSERT_MSG(out_entry->mapped_mem != MAP_FAILED, "Failed to map memory");
    return true;
#endif
}

bool file_read(Allocator* alloc, Path* path, DString* out_res)
{
    ASSERT_NON_NULL(out_res);
    path->ensure_null_term();

    sz size = file_get_size_from_path(path);
    if (!out_res->is_initialized()) out_res->init(alloc, size);
    
    // Decide if we want to memory map it (large files).
    if (size <= FILE_MEMORY_MAP_BOUNDARY)
    {
        if (!file_read_default(path, out_res, size)) return false;
    }
    else
    {
        if (!file_read_mmap(path, out_res, size)) return false;
    }

    ASSERT_MSG(out_res->byte_size_used() >= size, "Must've read equal to file size");
    return true;
}

intern bool file_read_default(Path* path, DString* out_data, sz precomputed_file_size)
{
    ASSERT_NON_NULL(path);
    ASSERT_INITIALIZED(out_data);

    FileHandle handle;
    if (!file_open(path, &handle, FileOpenBit::READ)) return false;
    defer(file_close(handle));

    if (precomputed_file_size == 0) precomputed_file_size = file_get_size(handle);
    out_data->reserve(precomputed_file_size);

    sz read_bytes_all = 0;
    sz read_bytes_curr = 0;
#ifdef RG_PLATFORM_WIN32
    do
    {
        if (!::ReadFile(handle, out_data->data + read_bytes_all, file_size, &read_bytes_curr))
        {
            LOG_ERROR("Failed to read file data from path: " FMT_STR_LEN, FMT_DSTRING(path));
            return false;
        }
        if (read_bytes_curr == 0)
        {
            ASSERT_MSG(read_bytes_all == precomputed_file_size, "Must be fully read");
            return true;
        }
        read_bytes_all += read_bytes_curr; 
    } while (read_bytes_all < file_size);
#else
    do
    {
        read_bytes_curr = ::read(handle, out_data->data + read_bytes_all, precomputed_file_size - read_bytes_all);
        if (read_bytes_curr < 0)
        {
            LOG_ERROR("Failed to read from file: " FMT_STR_LEN, FMT_DSTRING(path));
            return false;
        }
        if (read_bytes_curr == 0)
        {
            ASSERT_MSG(read_bytes_all == precomputed_file_size, "Must be fully read");
            return true;
        }
        read_bytes_all += read_bytes_curr; 
    } while (read_bytes_all < precomputed_file_size);
#endif
    return true;
}

intern bool file_read_mmap(Path* path, DString* out_data, sz precomputed_file_size)
{
    ASSERT_INITIALIZED(out_data);

    FileMapEntry mmap_entry;
    if (!file_memory_map(path, &mmap_entry, precomputed_file_size)) return false;
    defer(mmap_entry.unmap());

    out_data->reserve(mmap_entry.size);
    out_data->push(mmap_entry.slice());
    return true;
}

bool file_set_cursor(FileHandle handle, FileSeekPos pos, sz offset)
{
    ASSERT_MSG(handle != FILE_HANDLE_INVALID, "Must be valid file handle");

#ifdef RG_PLATFORM_WIN32
    if (!::SetFilePointerEx(handle, offset, null, s32(pos)))
    {
        LOG_ERROR("File seek failed");
        return false;
    }
#else
    if (::lseek(handle, offset, s32(pos)) < 0)
    {
        LOG_ERROR("File seek failed");
        return false;
    }
#endif
    return true;
}

bool file_get_cursor(FileHandle handle, sz* out_cursor)
{
#ifdef RG_PLATFORM_WIN32
    if (!::SetFilePointerEx(handle, 0, out_cursor, s32(FileSeekPos::CUR)))
    {
        LOG_ERROR("File seek failed");
        return false;
    }
#else
    sz cursor = ::lseek(handle, 0, s32(FileSeekPos::CUR));
    if (cursor < 0)
    {
        LOG_ERROR("File seek failed");
        return false;
    }
    *out_cursor = cursor;
#endif
    return true;
}

bool file_write(Path* path, Slice<u8> data, bool at_end)
{
    if (data.count > FILE_MEMORY_MAP_BOUNDARY) return file_write_mmap(path, data, at_end);
    else return file_write_default(path, data, at_end);
}

intern bool file_write_default(Path* path, Slice<u8> data, bool at_end)
{
    ASSERT_INITIALIZED(path);
    ASSERT_GREATER_ZERO(data.count);

    FileHandle handle;
    if (!file_open(path, &handle, FileOpenBit::WRITE)) return false;
    defer(file_close(handle));

    if (at_end) file_set_cursor(handle, FileSeekPos::END);

    sz written_bytes_all = 0;
    sz written_bytes_curr = 0;
#ifdef RG_PLATFORM_WIN32
    do
    {
        if (!::WriteFile(handle, data.at_ref(written_bytes_all), data.count - written_bytes_all, &written_bytes_curr, null);
        {
            LOG_ERROR("Failed to write to the file from path: " FMT_STR_LEN, FMT_DSTRING(path));
            return false;
        }
        if (written_bytes_curr == 0)
        {
            ASSERT_MSG(written_bytes_all >= data.count, "Must be all written");
            return true;
        }
        written_bytes_all += written_bytes_curr;
    } while (written_bytes_all < data.count);
#else
    do
    {
        written_bytes_curr = ::write(handle, data.at_ref(written_bytes_all), data.count - written_bytes_all);
        if (written_bytes_curr < 0)
        {
            LOG_ERROR("Failed to write to the file from path: " FMT_STR_LEN, FMT_DSTRING(path));
            return false;
        }
        if (written_bytes_curr == 0)
        {
            ASSERT_MSG(written_bytes_all >= data.count, "Must be all written");
            return true;
        }
        written_bytes_all += written_bytes_curr;
    } while (written_bytes_all < data.count);
#endif
    return true;
}

intern bool file_write_mmap(Path* path, Slice<u8> data, bool at_end)
{
    FileMapEntry mmap_entry;
    if (!file_memory_map(path, &mmap_entry)) return false;
    defer(mmap_entry.unmap());
    u8* start = mmap_entry.mapped_mem; 
    if (at_end) start += mmap_entry.size;
    mem_copy(start, data.ptr, data.count);
    return true;
}

void FileMapEntry::unmap()
{
#ifdef RG_PLATFORM_WIN32
    if (this->mapped_mem == null) return;
    ::UnmapViewOfFile(entry->mapped_mem);
    ::CloseHandle(entry->file_mapping);
    if (entry->file_handle != FILE_HANDLE_INVALID) ::CloseHandle(entry->file_handle);    
#else
    if (this->mapped_mem == null) return;
    ::munmap(this->mapped_mem, this->size);
#endif
    *this = {};
}

} // rg
