#include <stdarg.h>
#include <unistd.h>
#include "basic.hpp"
#include "slice.hpp"
#include "split_iterator.hpp"
#include "string.hpp"
#include "io.hpp"
#include "log.hpp"

namespace rg
{

// StrView & CStrView.

StrView::StrView(CString cstr)
{
    this->init(cstr, true);
}

StrView::StrView(CString cstr, sz count)
{
    this->ptr = cstr;
    this->count = count;
}

#ifdef RG_PLATFORM_WIN32
StrView::StrView(CString cstr, sz count, bool is_wide)
{
    this->ptr = cstr;
    this->count = count;
    this->is_wide = is_wide;
}
#endif

void StrView::init(CString cstr, bool preserve_null_term)
{
    this->ptr = cstr;
    this->count = strlen(cstr);
    if (preserve_null_term) this->count++;
}

bool StrView::contains_non_ascii()
{
    for (char c : *this)
    {
        if (c > 0x7F) return true;
    }
    return false;
}

void StrView::trim_until_null(bool inclusive)
{
    this->trim_until('\0', inclusive);
}

StrView StrView::slice_until_null(bool inclusive)
{
    Slice<const char> res = this->slice_until('\0', inclusive);
    return { res.ptr, res.count };
}

StrView utf8_to_utf16(Allocator* alloc, StrView str_view)
{
#ifdef RG_PLATFORM_WIN32
    s32 wide_len = MultiByteToWideChar(CP_UTF8, 0, str_view.ptr, str_view.count, NULL, 0);
    if (wide_len == 0)
    {
        return {};
    }

    sz new_size = (sizeof(wchar_t)) * wide_len;
    char* wide_str_out = (char*)allocator_allocate(alloc, new_size);

    if (MultiByteToWideChar(CP_UTF8, 0, str_view.ptr, str_view.count, wide_str_out, wide_len) == 0)
    {
        return { wide_str_out, new_size, true };
    }
    return {};
#else
    return str_view;
#endif
}

// DString.

void DString::init(Allocator* alloc, sz init_capacity)
{
    this->data = (char*)allocator_allocate(alloc, init_capacity * sizeof(char));
    this->count = 0;
    this->capacity = init_capacity;
    this->alloc = alloc;
}

void DString::init_cstr(Allocator* alloc, CString cstr, bool preserve_null_term)
{
    sz len = strlen(cstr);
    sz init_cap = max(DSTRING_DEFAULT_CAPACITY, len + 1);
    this->data = (char*)allocator_allocate(alloc, init_cap * sizeof(char));
    if (preserve_null_term) len += 1;
    this->count = len;
    this->capacity = capacity;
    this->alloc = alloc;
    mem_copy(this->data, (void*)cstr, len);
}

void DString::init_slice(Allocator* alloc, Slice<char> slice, sz additional_capacity)
{
    sz init_cap = max(DSTRING_DEFAULT_CAPACITY, slice.count + additional_capacity);
    this->data = (char*)allocator_allocate(alloc, init_cap * sizeof(char));
    this->count = 0;
    this->capacity = init_cap;
    this->alloc = alloc;
    if (slice.count)
    {
        this->push_many(slice);
    }
}

void DString::init_view(Allocator* alloc, StrView str_view, sz additional_capacity)
{
    sz init_cap = max(DSTRING_DEFAULT_CAPACITY, str_view.count + additional_capacity);
    this->data = (char*)allocator_allocate(alloc, init_cap * sizeof(char));
    this->count = 0;
    this->capacity = init_cap;
    this->alloc = alloc;
    if (str_view.count)
    {
        this->push(str_view);
    }
}

void DString::push(StrView str_view)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized first");
    ASSERT_MSG(str_view.is_valid(), "Must be valid string view");

    // Remove duplicated null terminator.
    if (this->is_null_term()) this->count--;

    this->reserve(str_view.count);

    char* start = this->data + this->count;
    for (char c : str_view)
    {
        *start = c; 
        ++start;
    }
}

void DString::push(Slice<char> slice)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized first");
    ASSERT_MSG(slice.is_valid(), "Must be valid slice");

    this->reserve(slice.count);

    char* start = this->data + this->count;
    for (char c : slice)
    {
        *start = c; 
        ++start;
    }
}

void DString::push(CString cstr)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized first");
    StrView str_view(cstr);
    this->push(str_view);
}

void DString::push(char c)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized first");
    this->reserve(1);
    *this->end() = c;
    this->count++;
}

void DString::push_fmt(CString fmt, ...)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized first");
    va_list args;
    va_start(args, fmt);
    s32 size = vsnprintf(null, 0, fmt, args);
    if (size > 0)
    {
        char* res = (char*)allocator_allocate(this->alloc, size);
        vsnprintf(res, size, fmt, args);
        StrView str_view{ res, size };
        this->push(str_view);
    }
    va_end(args);
}

CString DString::cstr()
{
    this->ensure_null_term();
    return (CString)this->data;
}

void DString::ensure_null_term()
{
    ASSERT_MSG(this->is_initialized(), "Requires allocator initialization");
    if (this->is_null_term()) return;
    this->push('\0');
}

void DString::ensure_no_null_term()
{
    if (this->is_empty() || !this->is_null_term()) return;
    this->count--;
}

void DString::foreach_codepoint(void(*fn)(Utf8Codepoint&))
{
    if (this->is_empty()) return;
    Utf8CodepointIterator iter = this->get_codepoint_iter();
    Utf8Codepoint point;
    
    while (true)
    {
        point = iter.next();
        if (point == UTF8_CODEPOINT_INVALID) return;
        fn(point);
    }
}

Utf8CodepointIterator DString::get_codepoint_iter()
{
    StrView view = this->view();
    Utf8CodepointIterator iter; 
    iter.view = view;
    iter.pos = 0;
    return iter;
}

u64 DString::hash()
{
    u64 hash = FNV_OFFSET_BASIS;
    sz byte_count = this->count;
    char* byte = this->data;
    char* end = byte + byte_count;

    for (; byte != end; ++byte)
    {
        hash ^= *byte;
        hash *= FNV_PRIME;
    }

    return hash;
}

bool operator==(DString& lhs, DString& rhs)
{
    if (lhs.count != rhs.count) return false;
    char* first = lhs.data;
    char* sec = rhs.data;
    char* end = first + lhs.count;

    for (; first != end; ++first, ++sec)
    {
        if (*first != *sec) return false;
    }
    return true;
}

// CodepointIterator.

inline u8 Utf8CodepointIterator::get_byte_at(sz offset)
{
    ASSERT_MSG(!this->is_at_end(), "Codepoint iterator mustn't reach the end");
    return this->view[this->pos + offset];
}

Utf8Codepoint Utf8CodepointIterator::next()
{
    Utf8Codepoint res = UTF8_CODEPOINT_INVALID;
    if (this->is_at_end()) return res;

    u8 byte = this->get_byte_at();
    s32 count_bytes = 0;

    // Single byte (ASCII)
    if ((byte & 0x80) == 0)
    {
        res = u32(byte);
        count_bytes = 1;
    }
    // 2 bytes
    else if ((byte & 0xE0) == 0xC0)
    {
        res = u32(byte & 0x1F);
        count_bytes = 2;
    }
    // 3 bytes
    else if ((byte & 0xF0) == 0xE0)
    {
        res = u32(byte & 0x0F);
        count_bytes = 3;
    }
    // 4 bytes
    else if ((byte & 0xF8) == 0xF0)
    {
        res = u32(byte & 0x07);
        count_bytes = 4;
    }
    // Invalid start byte
    else
    {
        return res;
    }

    // Read continuation bytes
    for (s32 i = 1; i < count_bytes; i++)
    {
        u8 next = this->get_byte_at(i);
        if ((next & 0xC0) != 0x80) {
            // Invalid continuation byte
            return res;
        }
        res = u32((res << 6) | (next & 0x3F));
    }

    this->step(count_bytes);
    return res;
}

// Logging

void log_proc(LogLevel level, CString fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fputs(stdout, LOG_INTROS[level]);
    vfprintf(stdout, fmt, args);
    fputs(LOG_COLOR_RESET, stdout);
    va_end(args);
}

// Path.

// NOTE: do we need to add a separator at end?
void Path::init_from_cwd(Allocator* alloc, sz add_cap)
{
    ASSERT_MSG(add_cap >= 0, "Must be equal or greater to 0");
#ifdef RG_PLATFORM_WIN32
    this->capacity = add_cap + 1;
    u32 size = GetCurrentDirectory(0, null);
    this->capacity += (sz)size;
    this->data = (char*)allocator_allocate(alloc, this->capacity);
    this->count = GetCurrentDirectory(this->count, this->ptr);
#else
    this->capacity = 256 + add_cap + 1;
    this->data = (char*)allocator_allocate(alloc, this->capacity);
    while (true)
    {
        this->data = getcwd(this->data, this->capacity);
        // Success.
        if (this->data) break;
        // Failure - grow the buffer.
        this->capacity *= 2;
        this->data = (char*)allocator_reallocate(alloc, this->data, this->capacity);
    };
    auto res{ this->view() };
    res.trim_until('\0', false);
    // How much space left in the allocated buffer.
    sz remain = this->capacity - res.count;
    if (remain < add_cap)
    {
        sz diff = add_cap - remain;
        this->capacity += diff;
        this->data = (char*)allocator_reallocate(alloc, this->data, this->capacity);
    }
    else res.count += add_cap;
    this->count = res.count;
#endif
    this->push(PATH_SEPARATOR);
}

void Path::init_absolute(Allocator* alloc, StrView path, bool null_term)
{
    this->init_from_cwd(alloc, path.count);
    ASSERT_MSG(this->last() == PATH_SEPARATOR, "Must be separated at end");
    ASSERT_MSG(this->last() != '\0', "Mustn't be null terminated");
    this->alloc = alloc;
    path.replace(PATH_SEPARATOR_INVALID, PATH_SEPARATOR);
    this->push(path);
    if (null_term) this->ensure_null_term();
}

void Path::init_absolute(Allocator* alloc, CString path, bool null_term)
{
    StrView str_view(path);
    this->init_absolute(alloc, str_view, null_term);
}

// Files.

enum struct FileOpenBit
{
    READ = 1 << 0,
    WRITE = 1 << 1,
    READ_WRITE = READ | WRITE,
};

inline s32 operator|(FileOpenBit a, FileOpenBit b) { return a | b; }
inline s32 operator&(FileOpenBit a, FileOpenBit b) { return a & b; }
FileOpenBit& operator|=(FileOpenBit& a, FileOpenBit b);

// TODO:
bool file_open(Path* path, FileHandle& handle_out, FileOpenBit flags)
{
#ifdef RG_PLATFORM_WIN32
    SECURITY_ATTRIBUTES sa;
    memory_zero(&sa, sizeof(SECURITY_ATTRIBUTES));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    DWORD windows_flags = 0;
    if (flags & FileOpenBit::READ)    windows_flags |= GENERIC_READ;
    if (flags & FileOpenBit::WRITE)   windows_flags |= GENERIC_WRITE;

    // TODO:
    if ()
    handle_out = CreateFileA(file_path,
        windows_flags,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        &sa,
        OPEN_EXISTING,
        NULL,
        NULL);
#else
    u32 unix_flags = O_RDONLY;
    if (u32(flags & FileOpenBit::WRITE))
    {
        if (u32(flags & FileOpenBit::READ)) unix_flags = O_RDWR;
        else unix_flags = O_WRONLY;
    }
    handle_out = ::open(path->cstr(), (s32)unix_flags);
#endif
    if (handle_out == FILE_HANDLE_INVALID) {
        report_error("Could not open file \"%s\"", error_string(file_path, is_wide));
        return false;
    }
    return true;
}

bool File::close()
{
    if (this->handle == FILE_HANDLE_INVALID) return false;
    #ifdef RG_PLATFORM_WIN32
        if (!CloseHandle(this->handle)) return false;
    #else
        if (this->handle <= 2) return false;  // Don't close stdin(0), stdout(1), stderr(2)
        if (::close(this->handle) != 0) return false;
    #endif // RG_PLATFORM_WIN32
    return true;
}

// bool file_open(Path* file_path, File* out_file, CString options = FILE_OPEN_OPTS[(sz)FileOpenOption::READ_WRITE])
// {
//     ASSERT(out_file != null);
//     out_file->handle = fopen(file_path->cstr(), options);
//     return out_file->handle != null;
// }

// void File::close()
// {
//     fclose(this->handle);
//     this->handle = null;
// }

// sz File::len()
// {
//     ASSERT_MSG(this->handle != null, "Must be valid file handle");
//     fseek(this->handle, 0, SEEK_END);
//     sz len = ftell(this->handle);
//     rewind(this->handle);
//     return len;
// }

// bool file_read_entire(Allocator* alloc, Path* path, DString* out_res)
// {
//     ASSERT(out_res != null);
    
//     path->ensure_null_term();
//     File file;
//     bool open_res = file_open(path, &file, FILE_OPEN_OPTS[(sz)FileOpenOption::READ]);
//     if (!open_res) return false;
//     sz flen = file.len();
//     out_res->init(alloc, flen);
//     sz res = fread(out_res->data, flen, 1, file.handle);
//     return res > 0;
// }

// bool file_read_entire_into(Path* path, StrView buff)
// {
//     path->ensure_null_term();
//     File file;
//     bool open_res = file_open(path, &file, FILE_OPEN_OPTS[(sz)FileOpenOption::READ]);
//     if (!open_res) return false;
//     sz flen = file.len();
//     ASSERT_MSG(buff.count >= flen, "Not enough memory in the buffer");
//     sz res = fread(buff.ptr, flen, 1, file.handle);
//     return res > 0;
// }

// // TODO:
// void file_write(Path path, StrView write_buff)
// {
    
// }

} // rg
