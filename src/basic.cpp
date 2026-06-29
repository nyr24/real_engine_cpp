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

void CStrView::init(CString cstr)
{
    this->ptr = cstr;
    this->count = strlen(cstr) + 1;
    ASSERT_MSG(this->last() == '\0', "Should be null-terminated view");
}

CStrView cstrview_create(CString cstr)
{
    CStrView res;
    res.init(cstr);
    return res;
}

// Will allocate memory and do copy memory, because this is modifiable view.
// Use CStrView for constant view without copying.
void StrView::init_cstr(Allocator* alloc, CString cstr)
{
    sz len = strlen(cstr);
    this->ptr = (char*)allocator_allocate(alloc, len * sizeof(char));
    this->count = len;
    mem_copy(this->ptr, (void*)cstr, len);
}

// Will allocate memory and do copy memory, because this is modifiable view.
// Use CStrView for constant view without copying.
StrView strview_create_cstr(Allocator* alloc, CString cstr)
{
    StrView res;
    res.init_cstr(alloc, cstr);
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

void DString::init(Allocator* alloc, sz init_capacity)
{
    this->data = (char*)allocator_allocate(alloc, init_capacity * sizeof(char));
    this->count = 0;
    this->capacity = init_capacity;
    this->alloc = alloc;
}

void DString::init_cstr(Allocator* alloc, CString cstr)
{
    sz len = strlen(cstr);
    // Allocate at minimum 1 more than length, for easier null termination.
    sz init_cap = max(DSTRING_DEFAULT_CAPACITY, len + 1);
    this->data = (char*)allocator_allocate(alloc, init_cap * sizeof(char));
    this->count = len;
    this->capacity = capacity;
    this->alloc = alloc;
    mem_copy(this->data, (void*)cstr, len);
}

void DString::init_with_values(Allocator* alloc, StrView init_values, sz additional_capacity)
{
    sz init_cap = max(DSTRING_DEFAULT_CAPACITY, init_values.count + additional_capacity);
    this->data = (char*)allocator_allocate(alloc, init_cap * sizeof(char));
    this->count = 0;
    this->capacity = init_cap;
    this->alloc = alloc;
    if (init_values.count)
    {
        this->push_many(init_values);
    }
}

void DString::init_with_values(Allocator* alloc, CStrView init_values, sz additional_capacity)
{
    sz init_cap = max(DSTRING_DEFAULT_CAPACITY, init_values.count + additional_capacity);
    this->data = (char*)allocator_allocate(alloc, init_cap * sizeof(char));
    this->count = 0;
    this->capacity = init_cap;
    this->alloc = alloc;
    if (init_values.count)
    {
        this->push_many(init_values);
    }
}

void DString::push_many(StrView str_view)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized first");
    ASSERT_MSG(str_view.ptr && str_view.count, "Must be valid string view");

    this->reserve(str_view.count);
    char* data = this->data;

    for (char c : str_view)
    {
        *(data + this->count) = c;
        this->count++;
    }
}

void DString::push_many(CStrView cstr_view)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized first");
    ASSERT_MSG(cstr_view.ptr && cstr_view.count, "Must be valid cstring view");

    // Remove duplicated null terminator.
    if (this->is_null_term()) this->count--;

    this->reserve(cstr_view.count);
    char* data = this->data;

    for (char c : cstr_view)
    {
        *(data + this->count) = c;
        this->count++;
    }
}

void DString::push_cstr(CString cstr)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized first");
    CStrView cstr_view = cstrview_create(cstr);
    // Remove duplicated null terminator.
    if (this->is_null_term()) this->count--;
    this->push_many(cstr_view);
}

void DString::ensure_null_term()
{
    ASSERT_MSG(this->is_initialized(), "Requires allocator initialization");
    if (this->is_null_term()) return;
    this->reserve(1);
    this->push('\0');
}

void DString::ensure_no_null_term()
{
    if (this->is_empty()) return;
    if (!this->is_null_term()) return;
    this->count--;
}

void DString::foreach_codepoint(void(*fn)(Codepoint&))
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

CodepointIterator DString::get_codepoint_iter()
{
    StrView view = this->view();
    CodepointIterator iter; 
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

inline u8 CodepointIterator::get_byte_at(sz offset)
{
    ASSERT_MSG(!this->is_at_end(), "Codepoint iterator mustn't reach the end");
    return this->view[this->pos + offset];
}

Maybe<Codepoint> CodepointIterator::next()
{
    if (this->is_at_end()) return maybe_empty<Codepoint>();

    u8 byte = this->get_byte_at();
    Codepoint codepoint;
    s32 count_bytes = 0;

    // Single byte (ASCII)
    if ((byte & 0x80) == 0) {
        codepoint = u32(byte);
        count_bytes = 1;
    }
    // 2 bytes
    else if ((byte & 0xE0) == 0xC0) {
        codepoint = u32(byte & 0x1F);
        count_bytes = 2;
    }
    // 3 bytes
    else if ((byte & 0xF0) == 0xE0) {
        codepoint = u32(byte & 0x0F);
        count_bytes = 3;
    }
    // 4 bytes
    else if ((byte & 0xF8) == 0xF0) {
        codepoint = u32(byte & 0x07);
        count_bytes = 4;
    }
    // Invalid start byte
    else {
        return maybe_empty<Codepoint>();
    }

    // Read continuation bytes
    for (s32 i = 1; i < count_bytes; i++) {
        u8 next = this->get_byte_at(i);
        if ((next & 0xC0) != 0x80) {
            // Invalid continuation byte
            return maybe_empty<Codepoint>();
        }
        codepoint = u32((codepoint << 6) | (next & 0x3F));
    }

    this->step(count_bytes);
    return maybe_create(codepoint);
}

} // rg
