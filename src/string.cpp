#include <cstdarg>
#include "string.hpp"

namespace rg
{

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

bool StrView::starts_with(StrView input)
{
    ASSERT_INITIALIZED(this);
    ASSERT_INITIALIZED_VAL(input);
    return common_starts_with<const char>(this->ptr, this->count, input);
}

bool StrView::starts_with(CString input)
{
    ASSERT_INITIALIZED(this);
    if (!input || *input == '\0') return false;

    const char* inp_curr = input;
    const char* curr = this->begin();
    const char* end = this->end();
    for (; *inp_curr != '\0' && curr != end && *inp_curr == *curr; ++inp_curr, ++curr)
    {
    }
    return *inp_curr == '\0';
}

bool contains_non_ascii(const char* start, const char* end)
{
    for (; start != end; ++start)
    {
        if (*start > 0x7F) return true;
    }
    return false;
}

bool is_space(char c)
{
    // Bits set: 9 (tab), 10 (LF), 11 (VT), 12 (FF), 13 (CR), 32 (Space)
    return (1ULL << c) & 0x200006200ULL;
}

void trim_space_start(const char** start, sz* count)
{
    const char* curr = *start;
    const char* end = curr + *count;

    while (curr != end && is_space(*curr)) { ++curr; }

    sz dist = curr - *start;
    if (dist)
    {
        *start += dist;
        *count -= dist;
    }
}

void trim_space_end(const char* start, sz* count)
{
    const char* end = start + *count - 1;
    const char* curr = end;
    while (curr != start && is_space(*curr)) { --curr; }

    sz dist = end - curr;
    if (dist)
    {
        *count -= dist;
    }
}

void trim_space_both(const char** start, sz* count)
{
    trim_space_start(start, count); 
    trim_space_end(*start, count); 
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
        this->push(slice);
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
    ASSERT_MSG(str_view.is_initialized(), "Must be valid string view");

    // Remove duplicated null terminator.
    if (this->is_null_term()) this->count--;

    this->reserve(str_view.count);
    char* copy_start = this->end();
    mem_copy(copy_start, (void*)str_view.ptr, str_view.count);
    this->count += str_view.count;
}

void DString::push(Slice<char> slice)
{
    ASSERT_INITIALIZED(this);
    ASSERT_INITIALIZED_VAL(slice);
    this->reserve(slice.count);
    char* copy_start = this->end();
    mem_copy(copy_start, slice.ptr, slice.count);
    this->count += slice.count;
}

void DString::push(Slice<u8> slice)
{
    ASSERT_INITIALIZED(this);
    ASSERT_INITIALIZED_VAL(slice);
    this->reserve(slice.count);
    char* copy_start = this->end();
    mem_copy(copy_start, slice.ptr, slice.count);
    this->count += slice.count;
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

bool operator==(const DString& lhs, const DString& rhs)
{
    if (lhs.count != rhs.count) return false;
    char* first = lhs.data;
    char* sec = rhs.data;
    return mem_compare(first, sec, lhs.count);
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

} // rg
