#ifndef _RG_STRING_HPP_
#define _RG_STRING_HPP_

#include "basic.hpp"
#include "darray.hpp"
#include "farray.hpp"
#include "slice.hpp"

namespace rg
{

// Utf8 CodepointIterator.

typedef u32 Utf8Codepoint;

constexpr sz DSTRING_DEFAULT_CAPACITY = 16;
constexpr u32 UTF8_CODEPOINT_INVALID = u32(-1);

struct Utf8CodepointIterator
{
    StrView view;
    sz pos;

    Utf8Codepoint next();
private:
    inline u8 get_byte_at(sz offset = 0);
    inline void step(sz count = 1) { this->pos += count; }
    inline bool is_at_end() { return this->pos >= this->view.count; }
};

// DString - dynamic string type.

struct DString : DArray<char>
{
    void init(Allocator* alloc, sz init_capacity = DSTRING_DEFAULT_CAPACITY);
    void init_slice(Allocator* alloc, Slice<char> slice, sz additional_capacity = 0);
    void init_view(Allocator* alloc, StrView str_view, sz additional_capacity = 0);
    void init_cstr(Allocator* alloc, CString cstr, bool preserve_null_term = true);
    void push(char c);
    void push(StrView str_view);
    void push(Slice<char> slice);
    void push(CString cstr);
    void push_fmt(CString fmt, ...);
    void ensure_null_term();
    void ensure_no_null_term();
    Utf8CodepointIterator get_codepoint_iter();
    void foreach_codepoint(void(*fn)(Utf8Codepoint&));
    u64 hash();

    CString cstr();
    // View is constant.
    inline StrView view() { return StrView{ this->data, this->count }; }
    // Slice is modifiable.
    inline Slice<char> slice() { return Slice{ this->data, this->count }; }
    inline bool is_null_term() { return this->count && this->last() == '\0'; }
};

bool operator==(DString& lhs, DString& rhs);

// FString - fixed string type.

constexpr sz FSTRING_DEFAULT_CAPACITY = 16;

template<sz CAPACITY = FSTRING_DEFAULT_CAPACITY>
struct FString : FArray<char, CAPACITY>
{
    FString() = default;
    FString(CString cstr);

    void init_view(StrView str_view);
    void init_slice(Slice<char> slice);
    void init_cstr(CString cstr);
    void push_many(StrView str_view);
    void push_many(Slice<char> slice);
    void push_cstr(CString cstr);
    bool ensure_null_term();
    void ensure_no_null_term();
    Utf8CodepointIterator get_codepoint_iter();
    void foreach_codepoint(void(*fn)(Utf8Codepoint&));
    u64 hash();

    // View is constant.
    inline StrView view() { return StrView{ this->data, this->count }; }
    // Slice is modifiable.
    inline Slice<char> slice() { return Slice{ this->data, this->count }; }
    inline bool is_null_term() { return this->count && this->last() == '\0'; }
};

template<sz CAPACITY>
FString<CAPACITY>::FString(CString cstr)
{
    this->init_cstr(cstr);
}

template<sz CAPACITY>
void FString<CAPACITY>::init_cstr(CString cstr)
{
    this->count = 0;
    this->push_cstr(cstr);
}

template<sz CAPACITY>
void FString<CAPACITY>::init_view(StrView str_view)
{
    this->count = 0;
    this->push_many(str_view);
}

template<sz CAPACITY>
void FString<CAPACITY>::init_slice(Slice<char> slice)
{
    this->count = 0;
    this->push_many(slice);
}

template<sz CAPACITY>
void FString<CAPACITY>::push_many(Slice<char> slice)
{
    ASSERT_MSG(slice.is_valid(), "Must be valid slice");
    ASSERT_MSG(this->remain() >= slice.count, "Must be enough space");

    char* start = this->data + this->count;
    for (char c : slice)
    {
        *start = c;
        ++start;
    }
}

template<sz CAPACITY>
void FString<CAPACITY>::push_many(StrView str_view)
{
    ASSERT_MSG(str_view.is_valid(), "Must be valid slice");
    ASSERT_MSG(this->remain() >= str_view.count, "Must be enough space");

    // Remove duplicated null terminator.
    if (this->is_null_term()) this->count--;

    char* start = this->data + this->count;
    for (char c : str_view)
    {
        *start = c;
        ++start;
    }
}

template<sz CAPACITY>
void FString<CAPACITY>::push_cstr(CString cstr)
{
    StrView str_view(cstr);
    this->push_many(str_view);
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
void FString<CAPACITY>::foreach_codepoint(void(*fn)(Utf8Codepoint&))
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
    u64 hash = FNV_OFFSET_BASIS;
    sz byte_count = this->count;
    char* byte = this->ptr;
    char* end = byte + byte_count;

    for (; byte != end; ++byte)
    {
        hash ^= *byte;
        hash *= FNV_PRIME;
    }

    return hash;
}

template<sz CAPACITY>
bool operator==(FString<CAPACITY>& lhs, FString<CAPACITY>& rhs)
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

} // rg

#endif // _RG_STRING_HPP_
