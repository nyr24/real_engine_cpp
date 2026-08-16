#ifndef _RG_STRING_HPP_
#define _RG_STRING_HPP_

#include "core/basic.hpp"
#include "collections/darray.hpp"
#include "collections/farray.hpp"
#include "collections/slice.hpp"
#include "collections/bits.hpp"

namespace rg
{

// Common code.

// StrView - view over a cstring, read-only.

struct StrView : Slice<const char>
{
    StrView() = default;
    StrView(CString cstr);
    StrView(CString cstr, sz count);
    void init(CString cstr, bool preserve_null_term = false);
    StrView slice_until_char(char c, bool inclusive = false);
    StrView slice_while_callback(bool(*)(char));
    StrView slice_while_callback_and_trim(bool(*)(char));
    StrView slice_until_callback(bool(*)(char));
    StrView slice_until_callback_and_trim(bool(*)(char));
    void trim_until_char(char c, bool inclusive = false);
    void trim_space_start();
    void trim_space_end();
    void trim_space_both();
    bool starts_with(StrView input) const;
    bool ends_with(StrView input) const;
    StrView view(sz start = 0, sz offset = -1) const;
    StrView view_idx(sz start = 0, sz end = -1) const;
    Maybe<sz> index_of(char c) const;
    Maybe<sz> index_of(StrView seq) const;
    Maybe<sz> last_index_of(char c) const;
    Maybe<sz> last_index_of(StrView seq) const;
    void trim_from_start_to_first_occur(char search, bool inclusive);
    void trim_from_start_to_last_occur(char search, bool inclusive);
    void trim_from_end_to_first_occur(char search, bool inclusive);
    void trim_from_end_to_last_occur(char search, bool inclusive);
    bool trim_sequence_start(StrView trim_seq);
    bool trim_sequence_end(StrView trim_seq);
    void skip_chars_threshold_start(char threshold);

    bool is_null_term() const { return this->count && this->last() == '\0'; }
    // Removes const qualifier from pointer, be careful.
    Slice<u8> to_byte_slice_unsafe() const { return { (u8*)this->ptr, this->count }; }
    // Removes const qualifier from pointer, be careful.
    Slice<char> to_char_slice_unsafe() const { return { (char*)this->ptr, this->count }; }
};

Maybe<sz> str_common_index_of(const char* RESTRICT start, sz count, StrView seq);
Maybe<sz> str_common_last_index_of(const char* RESTRICT start, sz count, StrView seq);
void str_common_trim_from_start_to_first_occur(const char** RESTRICT start, sz* count, char search, bool inclusive);
void str_common_trim_from_start_to_last_occur(const char** RESTRICT start, sz* count, char search, bool inclusive);
void str_common_trim_from_end_to_first_occur(const char** RESTRICT start, sz* count, char search, bool inclusive);
void str_common_trim_from_end_to_last_occur(const char** RESTRICT start, sz* count, char search, bool inclusive);
bool str_common_starts_with(const char* RESTRICT ptr, sz count, StrView seq);
bool str_common_ends_with(const char* RESTRICT ptr, sz count, StrView input);
bool str_common_trim_sequence_start(const char** RESTRICT ptr, sz* count, StrView trim_seq);
bool str_common_trim_sequence_end(const char** RESTRICT ptr, sz* count, StrView trim_seq);

inline StrView slice_to_str_view(Slice<char> slice)
{
    return { slice.ptr, slice.count };
}

// Char lookup.

enum struct CharType
{
    SPACE,
	DIGIT,
	ALPHA,
	ALPHA_NUM,
	UPPER,
	LOWER,
	EnumSize,
};

alias CharMask = BitInt<u8>;

constexpr EnumArray<u8, CharType> CHAR_MASKS = {
    0b1,
    0b10, 
    0b100, 
    0b1000, 
    0b10000,
    0b100000,
};

bool is_space(char c);
bool is_alpha(char c);
bool is_alpha_num(char c);
bool is_upper(char c);
bool is_lower(char c);
bool lookup_char_by_mask(char c, u8 mask);

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

    void init_view(Allocator* alloc, StrView str_view, sz additional_capacity = 0);
    void init_cstr(Allocator* alloc, CString cstr, bool preserve_null_term = false);
    void push(char c);
    void push(StrView str_view);
    void push(CString cstr);
    void push_fmt(CString fmt, ...);
    void ensure_null_term();
    void ensure_no_null_term();
    Utf8CodepointIterator get_codepoint_iter() const;
    void foreach_codepoint(void(*fn)(Utf8Codepoint&)) const;
    u64 hash() const;
    CString cstr();
    StrView view(sz start = 0, sz offset = -1) const; 
    StrView view_idx(sz start = 0, sz end = -1) const; 
    bool is_null_term() const { return this->count && this->last() == '\0'; }
    void trim_end_n(sz count);
    bool starts_with(StrView input) const;
    bool ends_with(StrView input) const;
    void trim_from_end_to_first_occur(char search, bool inclusive = false);
    void trim_from_end_to_last_occur(char search, bool inclusive = false);
    void replace(char find, char replace);
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
    FString(StrView slice);
    void init_slice(StrView slice);
    void init_view(StrView str_view);
    void init_cstr(CString cstr);
    void init_cstr_sized(CString cstr, sz size);
    void push(StrView str_view);
    void push_cstr(CString cstr);
    void push_cstr_sized(CString cstr, sz size);
    bool ensure_null_term();
    void ensure_no_null_term();
    Utf8CodepointIterator get_codepoint_iter() const;
    void foreach_codepoint(void(*fn)(Utf8Codepoint)) const;
    u64 hash() const;
    StrView view(sz start = 0, sz offset = -1) const;
    StrView view_idx(sz start = 0, sz end = -1) const;
    bool is_null_term() const { return this->count && this->last() == '\0'; }
    void trim_end_n(sz count);
    bool starts_with(StrView input) const;
    bool ends_with(StrView input) const;
    void replace(char find, char replace);
    void trim_from_end_to_first_occur(char search, bool inclusive = false);
    void trim_from_end_to_last_occur(char search, bool inclusive = false);
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
FString<CAPACITY>::FString(StrView view)
{
    this->count = 0;
    this->push(view);
}

template<sz CAPACITY>
void FString<CAPACITY>::init_view(StrView sv)
{
    this->count = 0;
    this->push(sv);
}

template<sz CAPACITY>
void FString<CAPACITY>::push(StrView sv)
{
    ASSERT_INITIALIZED_VAL(sv);
    ASSERT_MSG(this->remain() >= sv.count, "Must be enough space");

    // Remove null redundant null char.
    if (sv.is_null_term() && this->is_null_term()) this->count--;

    char* start = this->data + this->count;
    mem_copy(start, sv.ptr, sv.count);
    this->count += sv.count;
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
    ASSERT_INITIALIZED(this);
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
StrView FString<CAPACITY>::view(sz start, sz offset) const
{
    if (offset == -1) offset = this->count;
    ASSERT_MSG(start + offset <= this->count, "Mustn't exceed count");
    return { this->data + start, offset };
}

template<sz CAPACITY>
StrView FString<CAPACITY>::view_idx(sz start, sz end) const
{
    if (end == -1) end = this->count - 1;
    sz dist = (end - start) + 1;
    ASSERT_GREATER_ZERO(dist);
    ASSERT_MSG(start + dist <= this->count, "Mustn't exceed count");
    return { this->data + start, dist };
}

template<sz CAPACITY>
void FString<CAPACITY>::trim_end_n(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    common_trim_end_n(&this->data, &this->count, trim_count);
}

template<sz CAPACITY>
bool FString<CAPACITY>::starts_with(StrView input) const
{
    return common_starts_with(&this->data, this->count, input);
}

template<sz CAPACITY>
bool FString<CAPACITY>::ends_with(StrView input) const
{
    return common_ends_with(&this->data, this->count, input);
}

template<sz CAPACITY>
void FString<CAPACITY>::replace(char find, char replace)
{
    for (char& curr : *this)
    {
        if (curr == find) curr = replace;
    }
}

template<sz CAPACITY>
void FString<CAPACITY>::trim_from_end_to_first_occur(char search, bool inclusive)
{
    return str_common_trim_from_end_to_first_occur((const char**)&this->data, &this->count, search, inclusive);
}

template<sz CAPACITY>
void FString<CAPACITY>::trim_from_end_to_last_occur(char search, bool inclusive)
{
    return str_common_trim_from_end_to_last_occur((const char**)&this->data, &this->count, search, inclusive);
}

template<sz CAPACITY>
void FString<CAPACITY>::foreach_codepoint(void(*fn)(Utf8Codepoint)) const
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
Utf8CodepointIterator FString<CAPACITY>::get_codepoint_iter() const
{
    StrView view = this->view();
    Utf8CodepointIterator iter; 
    iter.view = view;
    iter.pos = 0;
    return iter;
}

template<sz CAPACITY>
u64 FString<CAPACITY>::hash() const
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

// Char lookup.

inline bool is_sign(char c) { return c == '-' || c == '+'; }
inline bool is_digit(char c) { return c >= '0'  && c <= '9'; }
inline bool is_space(char c) { return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f'; }
inline bool is_alpha(char c) { return c >= 'A' && c <= 'z'; }
inline bool is_upper(char c) { return c >= 'A' && c <= 'Z'; }
inline bool is_lower(char c) { return c >= 'a' && c <= 'z'; }
inline bool is_alpha_num(char c) { return is_alpha(c) || is_digit(c); }
inline bool is_digit_or_dot(char c) { return is_digit(c) || c == '.'; }
inline bool is_digit_or_sign(char c) { return is_digit(c) || is_sign(c); }
inline bool is_alpha_numeric(char c) { return is_alpha(c) || is_digit(c); }

// Simd.

constexpr sz CHAR_LANE_COUNT_128 = 16;
constexpr sz CHAR_LANE_COUNT_256 = 32;

} // rg

#endif // _RG_STRING_HPP_
