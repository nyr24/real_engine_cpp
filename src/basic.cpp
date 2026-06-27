#include "basic.hpp"
#include "view.hpp"
#include "string.hpp"
#include "cstring"

namespace rg
{

// StrView & CStrView.

StrView strview_create(char* ptr, sz count)
{
    return StrView{ ptr, count };
}

CStrView cstrview_create(CString cstr)
{
    CStrView res;
    res.ptr = cstr;
    res.count = strlen(cstr);
    return res;
}

StrView StrView::ensure_null_term(Allocator* alloc)
{
    if (this->is_null_term()) return *this;

    StrView str_view;
    str_view.count = this->count + 1;
    str_view.ptr = (char*)allocator_allocate(alloc, str_view.count);
    mem_copy(str_view.ptr, this->ptr, this->count);
    return str_view;
}

// DString.

DString* dstring_create(Allocator* alloc, sz init_capacity)
{
    DString* dstring = (DString*)allocator_allocate(alloc, sizeof(DString) + init_capacity * sizeof(char));
    dstring->count = 0;
    dstring->capacity = init_capacity;
    dstring->alloc = alloc;
    return dstring;
}

DString* dstring_create_cstr(Allocator* alloc, CString cstr, sz additional_cap)
{
    sz len = strlen(cstr);
    sz capacity = len + additional_cap;
    DString* dstring = (DString*)allocator_allocate(alloc, sizeof(DString) + capacity * sizeof(char));
    dstring->count = len;
    dstring->capacity = capacity;
    dstring->alloc = alloc;
    mem_copy(dstring->data(), (void*)cstr, len);
    return dstring;
}

DString* dstring_create_with_values(Allocator* alloc, StrView init_values)
{
    DString* dstring = (DString*)allocator_allocate(alloc, sizeof(DString) + init_values.count * sizeof(char));
    dstring->count = 0;
    dstring->capacity = init_values.count;
    dstring->alloc = alloc;
    
    if (init_values.count)
    {
        dstring->push_many(init_values);
    }
    return dstring;
}

void DString::push_many(StrView str_view)
{
    ASSERT_MSG(str_view.ptr && str_view.count, "Must be valid string view");

    this->reserve(str_view.count);
    char* data = this->data();

    for (char c : str_view)
    {
        *(data + this->count) = c;
        this->count++;
    }
}

void DString::push_many(CStrView cstr_view)
{
    ASSERT_MSG(cstr_view.ptr && cstr_view.count, "Must be valid cstring view");

    this->reserve(cstr_view.count);
    char* data = this->data();

    for (char c : cstr_view)
    {
        *(data + this->count) = c;
        this->count++;
    }
}

void DString::push_cstr(CString cstr)
{
    CStrView cstr_view = cstrview_create(cstr);
    this->push_many(cstr_view);
}

void DString::ensure_null_term()
{
    ASSERT_MSG(this->is_initialized(), "Requires allocator initialization");
    if (this->is_null_term()) return;
    this->reserve(1);
    *this->end() = '\0';
}

void DString::ensure_no_null_term()
{
    if (!this->is_null_term()) return;
    this->count--;
}

void DString::foreach_codepoint(void(*fn)(Codepoint&))
{
    if (this->is_empty()) return;
    CodepointIterator iter = get_codepoint_iter();
    Maybe<Codepoint> point;
    
    while (true)
    {
        point = iter.next();
        if (!point) return;
        fn(point.val);
    }
}

// CodepointIterator.

CodepointIterator DString::get_codepoint_iter()
{
    StrView view = this->view();
    CodepointIterator iter; 
    iter.view = view;
    iter.pos = 0;
    return iter;
}

inline u8 CodepointIterator::curr_byte_and_move()
{
    ASSERT_MSG(!this->is_at_end(), "Codepoint iterator mustn't reach the end");
    return this->view[this->pos++];
}

Maybe<Codepoint> CodepointIterator::next()
{
    if (this->is_at_end()) return maybe_empty<Codepoint>();

    u8 byte = this->curr_byte_and_move();
    Codepoint codepoint;
    s32 num_bytes = 0;

    // Single byte (ASCII)
    if ((byte & 0x80) == 0) {
        codepoint = u32(byte);
        num_bytes = 1;
    }
    // 2 bytes
    else if ((byte & 0xE0) == 0xC0) {
        codepoint = u32(byte & 0x1F);
        num_bytes = 2;
    }
    // 3 bytes
    else if ((byte & 0xF0) == 0xE0) {
        codepoint = u32(byte & 0x0F);
        num_bytes = 3;
    }
    // 4 bytes
    else if ((byte & 0xF8) == 0xF0) {
        codepoint = u32(byte & 0x07);
        num_bytes = 4;
    }
    // Invalid start byte
    else {
        return maybe_empty<Codepoint>();
    }

    // Read continuation bytes
    for (s32 i = 1; i < num_bytes; i++) {
        u8 next = this->curr_byte_and_move();
        if ((next & 0xC0) != 0x80) {
            // Invalid continuation byte
            return maybe_empty<Codepoint>();
        }
        codepoint = u32((codepoint << 6) | (next & 0x3F));
    }

    this->pos += num_bytes;
    return maybe_create(codepoint);
}

} // rg
