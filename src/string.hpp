#ifndef _RG_STRING_HPP_
#define _RG_STRING_HPP_

#include "basic.hpp"
#include "darray.hpp"
#include "farray.hpp"
#include "view.hpp"

namespace rg
{

// CodepointIterator.

typedef u32 Codepoint;

const sz DSTRING_DEFAULT_CAPACITY = 16;

struct CodepointIterator
{
    StrView view;
    sz pos;

    Maybe<Codepoint> next();
private:
    inline u8 get_byte_at(sz offset = 0);
    inline void step(sz count = 1) { this->pos += count; }
    inline bool is_at_end() { return this->pos >= this->view.count; }
};

// DString - dynamic string type.

struct DString : DArray<char>
{
    void init(Allocator* alloc, sz init_capacity = DSTRING_DEFAULT_CAPACITY);
    void init_cstr(Allocator* alloc, CString cstr);
    void init_with_values(Allocator* alloc, StrView init_values, sz additional_capacity = 0);
    void init_with_values(Allocator* alloc, CStrView init_values, sz additional_capacity = 0);
    void push_many(StrView str_view);
    void push_many(CStrView cstr_view);
    void push_cstr(CString cstr);
    void ensure_null_term();
    void ensure_no_null_term();
    CodepointIterator get_codepoint_iter();
    void foreach_codepoint(void(*fn)(Codepoint&));
    u64 hash();

    inline StrView view() { return StrView{ this->data, this->count }; }
    inline bool is_null_term() { return this->count && this->last() == '\0'; }
};

bool operator==(DString& lhs, DString& rhs);

// FString - fixed string type.

constexpr sz FSTRING_DEFAULT_CAPACITY = 16;

template<sz CAPACITY = FSTRING_DEFAULT_CAPACITY>
struct FString : FArray<char, CAPACITY>
{
    void init_cstr(CString cstr);
    void init_with_values(StrView str_view);
    void init_with_values(CStrView str_view);
    void push_many(StrView str_view);
    void push_many(CStrView cstr_view);
    void push_cstr(CString cstr);
    bool ensure_null_term();
    void ensure_no_null_term();
    CodepointIterator get_codepoint_iter();
    void foreach_codepoint(void(*fn)(Codepoint&));
    u64 hash();

    inline StrView view() { return StrView{ this->data, this->count }; }
    inline bool is_null_term() { return this->count && this->last() == '\0'; }
};

template<sz CAPACITY>
void FString<CAPACITY>::init_cstr(CString cstr)
{
    this->count = 0;
    this->push_cstr(cstr);
}

template<sz CAPACITY>
void FString<CAPACITY>::init_with_values(StrView init_values)
{
    this->count = 0;
    this->push_many(init_values);
}

template<sz CAPACITY>
void FString<CAPACITY>::push_many(StrView str_view)
{
    ASSERT_MSG(str_view.is_valid(), "Must be valid string view");
    ASSERT_MSG(this->remain() >= str_view.count, "Must have enough space");

    char* data = this->data;

    for (char c : str_view)
    {
        *(data + this->count) = c;
        this->count++;
    }
}

template<sz CAPACITY>
void FString<CAPACITY>::push_many(CStrView cstr_view)
{
    ASSERT_MSG(cstr_view.is_valid(), "Must be valid cstring view");
    ASSERT_MSG(cstr_view.count <= this->remain(), "Must be enough space");

    // Remove duplicated null terminator.
    if (this->is_null_term()) this->count--;

    char* data = this->data;

    for (char c : cstr_view)
    {
        *(data + this->count) = c;
        this->count++;
    }
}

template<sz CAPACITY>
void FString<CAPACITY>::push_cstr(CString cstr)
{
    CStrView cstr_view = cstrview_create(cstr);
    this->push_many(cstr_view);
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
void FString<CAPACITY>::foreach_codepoint(void(*fn)(Codepoint&))
{
    if (this->is_empty()) return;
    CodepointIterator iter = this->get_codepoint_iter();
    Maybe<Codepoint> point;
    
    while (true)
    {
        point = iter.next();
        if (!point) return;
        fn(point.val);
    }
}

template<sz CAPACITY>
CodepointIterator FString<CAPACITY>::get_codepoint_iter()
{
    StrView view = this->view();
    CodepointIterator iter; 
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
