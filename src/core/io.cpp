#include "collections/string.hpp"
#include "core/basic.hpp"
#include "core/io.hpp"
#include "collections/slice.hpp"

namespace rg
{

// Path.

intern void platform_process_cwd(Path* path, Allocator* alloc, sz add_cap = 0);
intern sz get_steps_back_from_cwd_and_trim(StrView path);
intern void path_trim_parts_from_end(Path* path, sz trim_count);

void Path::init(Allocator* alloc, StrView init_part, bool null_term)
{
    this->alloc = alloc;

    if (path_is_absolute(init_part))
    {
        sz capacity = init_part.count;
        this->init_capacity(alloc, capacity);
        this->push(init_part);
        this->ensure_separator_at_end();
    }
    else
    {
        sz add_cap = init_part.count;
        platform_process_cwd(this, alloc, add_cap);
        this->add_part(init_part);
    }
    if (null_term) this->ensure_null_term();
}

void Path::init(Allocator* alloc, Slice<StrView> parts, bool null_term)
{
    this->alloc = alloc;

    if (path_is_absolute(parts[0]))
    {
        sz capacity = parts[0].count;
        for (StrView* part = parts.at_ref(1); part != parts.end(); ++part)
        {
           capacity += part->count;
        }
        this->init_capacity(alloc, capacity);
        this->push(parts[0]);
        this->ensure_separator_at_end();
        this->add_parts(parts.slice(1));
    }
    else
    {
        StrView first_part = parts[0];
        sz add_cap = first_part.count;
        for (StrView* part = parts.at_ref(1); part != parts.end(); ++part)
        {
            add_cap += part->count;
        }
        platform_process_cwd(this, alloc, add_cap);
        this->add_parts(parts);
    }
    if (null_term) this->ensure_null_term();
}

// First part can contain relative offsets: "./", "../", "../../"
void Path::add_parts(Slice<StrView> parts)
{
    ASSERT_INITIALIZED(this);

    StrView first_part = parts[0];
    sz steps_back_from_cwd = get_steps_back_from_cwd_and_trim(first_part);
    if (steps_back_from_cwd) path_trim_parts_from_end(this, steps_back_from_cwd);
    this->ensure_separator_at_end();

    for (StrView part : parts)
    {
        if (part.first() == PATH_SEPARATOR) part.trim_start_n(1);
        if (part.last() == PATH_SEPARATOR) part.trim_end_n(1);
        this->push(part);
        this->push(PATH_SEPARATOR);
    }
}

void Path::add_part(StrView part)
{
    ASSERT_INITIALIZED(this);

    sz steps_back_from_cwd = get_steps_back_from_cwd_and_trim(part);
    if (steps_back_from_cwd) path_trim_parts_from_end(this, steps_back_from_cwd);
    this->ensure_separator_at_end();
    if (part.first() == PATH_SEPARATOR) part.trim_start_n(1);
    if (part.last() == PATH_SEPARATOR) part.trim_end_n(1);
    this->push(part);
    this->push(PATH_SEPARATOR);
}

intern void path_trim_parts_from_end(Path* self, sz trim_count)
{
    for (; trim_count > 1; --trim_count)
    {
        self->trim_from_end_to_last_occur(PATH_SEPARATOR);
    }
}

void Path::ensure_separator_at_end()
{
    if (this->last() != PATH_SEPARATOR) this->push(PATH_SEPARATOR);
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
        path->count = ::GetCurrentDirectory(path->capacity, path->ptr);
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
        // Grow allocation.
        if (remain < add_cap)
        {
            sz diff = add_cap - remain;
            sz new_capacity = path->capacity + diff;
            path->data = (char*)allocator_reallocate(alloc, path->data, new_capacity);
            path->capacity = new_capacity;
        }
        // Shrink allocation if possible.
        else if (remain > add_cap)
        {
            path->data = (char*)allocator_reallocate(alloc, path->data, path->count);
            path->capacity = path->count;
        }
    #endif
}

// Returns a number of steps it needs to take from the cwd.
// . == 0, ./ == 0, ../ == 1, ../../ == 2, etc.
// if it doesn't start with a dot - returns 0
intern sz get_steps_back_from_cwd_and_trim(StrView input_path)
{
    if (path_is_absolute(input_path)) return 0;
    if (input_path.first() != '.') return 0;

    if (common_starts_with(input_path.ptr, input_path.count, RELATIVE_DIR_SELF))
    {
        input_path.trim_start_n(2);
        return 0;
    }

    sz steps = 0;
    if (common_starts_with(input_path.ptr, input_path.count, RELATIVE_DIR_PARENT))
    {
        input_path.trim_start_n(3);
        ++steps;
    }

    ASSERT_MSG(input_path.first() != PATH_SEPARATOR, "Path separators should be trimmed");
    return steps;
}

inline bool path_is_absolute(StrView path)
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

intern Maybe<DString> file_read_default(Allocator* alloc, Path* path, sz precomputed_file_size = 0);
intern Maybe<DString> file_read_mmap(Allocator* alloc, Path* path, sz precomputed_file_size = 0);
intern bool file_write_default(Path* path, Slice<u8> data, bool at_end = true);
intern bool file_write_mmap(Path* path, Slice<u8> data, bool at_end = true);

Maybe<FileHandle> file_open(Path* path, FileOpenBit flags)
{
    ASSERT_INITIALIZED(path);
    path->ensure_null_term();

    Maybe<FileHandle> res;

#ifdef RG_PLATFORM_WIN32
    DWORD windows_flags = 0;
    if (flags & FileOpenBit::READ)    windows_flags |= GENERIC_READ;
    if (flags & FileOpenBit::WRITE)   windows_flags |= GENERIC_WRITE;

    if (contains_non_ascii(path->data, path->end()))
    {
        path->convert_to_utf16();
        res.val = ::CreateFileW(path->data,
            windows_flags,
            GENERIC_READ | GENERIC_WRITE,
            null,
            OPEN_EXISTING,
            null,
            null);
    }
    else
    {
        res.val = ::CreateFileA(file_path,
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
    res.val = ::open(path->cstr(), (s32)unix_flags);
#endif
    if (res.val == HANDLE_INVALID)
    {
        LOG_ERROR("Could not open file from path: " FMT_PLACEHOLDER_LEN, FMT_DSTRING(path));
        return res;
    }
    res.is_ok = true;
    return res;
}

bool file_close(FileHandle handle)
{
    if (handle == HANDLE_INVALID)
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
    handle = HANDLE_INVALID;
    return true;
}

sz file_get_size(FileHandle handle)
{
    ASSERT_MSG(handle != HANDLE_INVALID, "Must be valid file handle");
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

    auto [handle, is_open] = file_open(path, FileOpenBit::READ_WRITE);
    if (!is_open) return false;
    defer(file_close(handle));

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
    auto [handle, is_open] = file_open(path, FileOpenBit::READ_WRITE);
    if (!is_open) return false;
    defer(file_close(handle));

    if (file_size == 0) file_size = file_get_size(handle);
    out_entry->size = file_size;
    out_entry->mapped_mem = (u8*)::mmap(null, out_entry->size, PROT_READ | PROT_WRITE, MAP_PRIVATE, handle, 0);
    ASSERT_MSG(out_entry->mapped_mem != MAP_FAILED, "Failed to map memory");
    return true;
#endif
}

Maybe<DString> file_read(Allocator* alloc, Path* path)
{
    path->ensure_null_term();

    sz size = file_get_size_from_path(path);
    Maybe<DString> res;
    
    // Decide if we want to memory map it (large files).
    if (size <= FILE_MEMORY_MAP_BOUNDARY)
    {
        res = file_read_default(alloc, path, size);
    }
    else
    {
        res = file_read_mmap(alloc, path, size);
    }
    return res;
}

intern Maybe<DString> file_read_default(Allocator* alloc, Path* path, sz precomputed_file_size)
{
    ASSERT_NON_NULL(path);

    Maybe<DString> res;
    auto [handle, is_opened] = file_open(path, FileOpenBit::READ);
    if (!is_opened) return res;
    defer(file_close(handle));

    if (precomputed_file_size == 0) precomputed_file_size = file_get_size(handle);
    res.val.init_capacity(alloc, precomputed_file_size);

    sz read_bytes_all = 0;
    sz read_bytes_curr = 0;
    defer(res.val.count = read_bytes_all);

#ifdef RG_PLATFORM_WIN32
    do
    {
        if (!::ReadFile(handle, out_data->data + read_bytes_all, file_size, &read_bytes_curr))
        {
            LOG_ERROR("Failed to read file data from path: " FMT_STR_LEN, FMT_DSTRING(path));
            return res;
        }
        if (read_bytes_curr == 0)
        {
            ASSERT_MSG(read_bytes_all == precomputed_file_size, "Must be fully read");
            res.is_ok = true;
            return res;
        }
        read_bytes_all += read_bytes_curr; 
    } while (read_bytes_all < file_size);
#else
    do
    {
        read_bytes_curr = ::read(handle, res.val.data + read_bytes_all, precomputed_file_size - read_bytes_all);
        if (read_bytes_curr < 0)
        {
            LOG_ERROR("Failed to read from file: " FMT_PLACEHOLDER_LEN, FMT_DSTRING(path));
            return res;
        }
        if (read_bytes_curr == 0)
        {
            ASSERT_MSG(read_bytes_all == precomputed_file_size, "Must be fully read");
            res.is_ok = true;
            return res;
        }
        read_bytes_all += read_bytes_curr; 
    } while (read_bytes_all < precomputed_file_size);
#endif
    res.is_ok = true;
    return res;
}

intern Maybe<DString> file_read_mmap(Allocator* alloc, Path* path, sz precomputed_file_size)
{
    Maybe<DString> res;
    FileMapEntry mmap_entry;
    if (!file_memory_map(path, &mmap_entry, precomputed_file_size)) return res;
    defer(mmap_entry.unmap());

    res.val.init_capacity(alloc, mmap_entry.size);
    res.val.push(mmap_entry.slice());
    res.is_ok = true;
    return res;
}

bool file_set_cursor(FileHandle handle, FileSeekPos pos, sz offset)
{
    ASSERT_MSG(handle != HANDLE_INVALID, "Must be valid file handle");

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

    auto [handle, is_open] = file_open(path, FileOpenBit::WRITE);
    if (!is_open) return false;
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
            LOG_ERROR("Failed to write to the file from path: " FMT_PLACEHOLDER_LEN, FMT_DSTRING(path));
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
    if (entry->file_handle != HANDLE_INVALID) ::CloseHandle(entry->file_handle);    
#else
    if (this->mapped_mem == null) return;
    ::munmap(this->mapped_mem, this->size);
#endif
    *this = {};
}

} // rg
