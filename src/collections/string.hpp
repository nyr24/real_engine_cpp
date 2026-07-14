#ifndef _RG_STRING_HPP_
#define _RG_STRING_HPP_

#include "core/basic.hpp"
#include "collections/darray.hpp"
#include "collections/farray.hpp"
#include "collections/slice.hpp"

namespace rg
{

// StrView - view over a cstring, read-only.

struct StrView : Slice<const char>
{
    StrView() = default;
    StrView(CString cstr);
    StrView(CString cstr, sz count);
    void init(CString cstr, bool preserve_null_term = true);
    void trim_until_null(bool inclusive = true);
    bool starts_with(StrView input);
    bool starts_with(CString input);
    // Removes const qualifier from pointer, be careful.
    Slice<u8> to_byte_slice_unsafe() { return { (u8*)this->ptr, this->count }; }
    // Removes const qualifier from pointer, be careful.
    Slice<char> to_char_slice_unsafe() { return { (char*)this->ptr, this->count }; }
};

inline StrView slice_to_str_view(Slice<char> slice)
{
    return { slice.ptr, slice.count };
}

inline bool is_digit(char c) { return c >= '0'  && c <= '9'; }
bool contains_non_ascii(const char* start, const char* end);
void trim_space_start(const char** start, sz* count);
void trim_space_end(const char* start, sz* count);
void trim_space_both(const char** start, sz* count);

#define FMT_STR_VIEW(str_view) (s32)str_view.count, str_view.ptr
#define FMT_STR_VIEW_PTR(str_view) (s32)str_view->count, str_view->ptr

// Utf8 CodepointIterator.

typedef u32 Utf8Codepoint;

constexpr u32 UTF8_CODEPOINT_INVALID = u32(-1);

struct Utf8CodepointIterator
{
    StrView view;
    sz pos;

    Utf8Codepoint next();
private:
    u8 get_byte_at(sz offset = 0);
    void step(sz count = 1) { this->pos += count; }
    bool is_at_end() { return this->pos >= this->view.count; }
};

// DString - dynamic string type.

struct DString : DArray<char>
{
    static constexpr sz DEFAULT_CAPACITY = 16;
    using DArray<char>::DArray;
    using DArray<char>::init;
    using DArray<char>::init_capacity;
    using DArray<char>::init_slice;
    void init_view(Allocator* alloc, StrView str_view, sz additional_capacity = 0);
    void init_cstr(Allocator* alloc, CString cstr, bool preserve_null_term = true);
    void push(char c);
    void push(StrView str_view);
    void push(Slice<char> slice);
    void push(Slice<u8> slice);
    void push(CString cstr);
    void push_fmt(CString fmt, ...);
    void ensure_null_term();
    void ensure_no_null_term();
    Utf8CodepointIterator get_codepoint_iter();
    void foreach_codepoint(void(*fn)(Utf8Codepoint&));
    u64 hash();
    CString cstr();
    StrView view(sz start = 0, sz offset = -1);
    StrView view_idx(sz start = 0, sz end = -1);
    bool is_null_term() { return this->count && this->last() == '\0'; }
};

bool operator==(const DString& lhs, const DString& rhs);

// For printf formatting with length (%.*s).
#define FMT_DSTRING(dstr) (s32)dstr->count, dstr->data
#define FMT_DSTRING_VAL(dstr) (s32)dstr.count, dstr.data

// FString - fixed string type.

constexpr sz FSTRING_DEFAULT_CAPACITY = 16;

template<sz CAPACITY = FSTRING_DEFAULT_CAPACITY>
struct FString : FArray<char, CAPACITY>
{
    using FArray<char, CAPACITY>::FArray;
    FString() = default;
    FString(CString cstr);
    FString(CString cstr, sz size);
    FString(Slice<char> slice);
    FString(StrView view);
    void init_slice(Slice<char> slice);
    void init_view(StrView str_view);
    void init_cstr(CString cstr);
    void init_cstr_sized(CString cstr, sz size);
    void push(StrView str_view);
    void push(Slice<char> slice);
    void push_cstr(CString cstr);
    void push_cstr_sized(CString cstr, sz size);
    bool ensure_null_term();
    void ensure_no_null_term();
    Utf8CodepointIterator get_codepoint_iter();
    void foreach_codepoint(void(*fn)(Utf8Codepoint));
    u64 hash();
    StrView view(sz start = 0, sz offset = -1);
    StrView view_idx(sz start = 0, sz end = -1);
    bool is_null_term() { return this->count && this->last() == '\0'; }
};

template<sz CAPACITY>
FString<CAPACITY>::FString(CString cstr)
{
    this->count = 0;
    this->push_cstr(cstr);
}

template<sz CAPACITY>
FString<CAPACITY>::FString(CString cstr, sz size)
{
    this->count = 0;
    this->push_cstr_sized(cstr, size);
}

template<sz CAPACITY>
FString<CAPACITY>::FString(Slice<char> slice)
{
    this->count = 0;
    this->push(slice);
}

template<sz CAPACITY>
FString<CAPACITY>::FString(StrView view)
{
    this->count = 0;
    this->push(view);
}

template<sz CAPACITY>
void FString<CAPACITY>::init_view(StrView str_view)
{
    this->count = 0;
    this->push(str_view);
}

template<sz CAPACITY>
void FString<CAPACITY>::init_slice(Slice<char> slice)
{
    this->count = 0;
    this->push(slice);
}

template<sz CAPACITY>
void FString<CAPACITY>::push(Slice<char> slice)
{
    ASSERT_INITIALIZED_VAL(slice);
    ASSERT_MSG(this->remain() >= slice.count, "Must be enough space");

    char* start = this->data + this->count;
    mem_copy(start, slice.ptr, slice.count);
    this->count += slice.count;
}

template<sz CAPACITY>
void FString<CAPACITY>::push(StrView str_view)
{
    ASSERT_INITIALIZED_VAL(str_view);
    ASSERT_MSG(this->remain() >= str_view.count, "Must be enough space");

    // Remove duplicated null terminator.
    if (this->is_null_term()) this->count--;

    char* start = this->data + this->count;
    mem_copy(start, (void*)str_view.ptr, str_view.count);
    this->count += str_view.count;
}

template<sz CAPACITY>
void FString<CAPACITY>::push_cstr(CString cstr)
{
    StrView str_view(cstr);
    this->push(str_view);
}

template<sz CAPACITY>
void FString<CAPACITY>::push_cstr_sized(CString cstr, sz size)
{
    StrView str_view(cstr, size);
    this->push(str_view);
}

template<sz CAPACITY>
bool FString<CAPACITY>::ensure_null_term()
{
    ASSERT_MSG(this->is_initialized(), "Requires allocator initialization");
    if (this->is_null_term()) return true;

    sz remain = this->remain();
    ASSERT_MSG(remain >= 1, "Must have a space for null character");
    if (remain < 1) return false;
    this->push('\0');
    return true;
}

template<sz CAPACITY>
void FString<CAPACITY>::ensure_no_null_term()
{
    if (this->is_empty()) return;
    if (!this->is_null_term()) return;
    this->count--;
}

template<sz CAPACITY>
StrView FString<CAPACITY>::view(sz start, sz offset)
{
    if (offset == -1) offset = this->count;
    ASSERT_MSG(start + offset <= this->count, "Mustn't exceed count");
    return { this->data + start, offset };
}

template<sz CAPACITY>
StrView FString<CAPACITY>::view_idx(sz start, sz end)
{
    if (end == -1) end = this->count - 1;
    sz dist = (end - start) + 1;
    ASSERT_GREATER_ZERO(dist);
    ASSERT_MSG(start + dist <= this->count, "Mustn't exceed count");
    return { this->data + start, dist };
}

template<sz CAPACITY>
void FString<CAPACITY>::foreach_codepoint(void(*fn)(Utf8Codepoint))
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

template<sz CAPACITY>
Utf8CodepointIterator FString<CAPACITY>::get_codepoint_iter()
{
    StrView view = this->view();
    Utf8CodepointIterator iter; 
    iter.view = view;
    iter.pos = 0;
    return iter;
}

template<sz CAPACITY>
u64 FString<CAPACITY>::hash()
{
    return rg::hash_fnv(this->data, this->count);
}

template<sz CAPACITY>
bool operator==(const FString<CAPACITY>& lhs, const FString<CAPACITY>& rhs)
{
    if (lhs.count != rhs.count) return false;
    auto* first = lhs.data;
    auto* sec = rhs.data;
    return mem_compare((void*)first, (void*)sec, lhs.count);
}

// For printf formatting with length (%.*s).
#define FMT_FSTRING(fstr) (s32)fstr->count, fstr->data
#define FMT_FSTRING_VAL(fstr) (s32)fstr.count, fstr.data

} // rg

#endif // _RG_STRING_HPP_
