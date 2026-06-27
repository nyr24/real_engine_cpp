#ifndef _RG_STRING_HPP_
#define _RG_STRING_HPP_

#include "basic.hpp"
#include "dynarr.hpp"
#include "view.hpp"

namespace rg
{

typedef u32 Codepoint;

const sz DSTRING_DEFAULT_CAPACITY = 16;

struct CodepointIterator
{
    StrView view;
    sz pos;

    Maybe<Codepoint> next();
    inline u8 curr_byte_and_move();
    inline bool is_at_end() { return this->pos >= this->view.count; }
};

struct DString : DArray<char>
{
    void push_many(StrView str_view);
    void push_many(CStrView cstr_view);
    void push_cstr(CString cstr);
    void ensure_null_term();
    void ensure_no_null_term();
    CodepointIterator get_codepoint_iter();
    void foreach_codepoint(void(*fn)(Codepoint&));

    inline StrView view() { return StrView{ this->data(), this->count }; }
    inline bool is_null_term() { return this->last() == '\0'; }
};

DString* dstring_create(Allocator* alloc, sz init_capacity = DSTRING_DEFAULT_CAPACITY);
DString* dstring_create_cstr(Allocator* alloc, CString cstr, sz additional_cap = 0);
DString* dstring_create_with_values(Allocator* alloc, StrView init_values = {});

} // rg

#endif // _RG_STRING_HPP_
