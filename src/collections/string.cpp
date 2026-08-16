#include <stdarg.h>
#include "collections/string.hpp"
#include "core/context.hpp"

#if defined(RG_FEATURE_SIMD_256) || defined(RG_FEATURE_SIMD_128)
    #include <immintrin.h>
#endif

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
    this->count = ::strlen(cstr);
    if (preserve_null_term) this->count++;
}

bool StrView::starts_with(StrView input) const
{
    ASSERT_INITIALIZED(this);
    ASSERT_INITIALIZED_VAL(input);
    return str_common_starts_with(this->ptr, this->count, input);
}

bool StrView::ends_with(StrView input) const
{
    ASSERT_INITIALIZED(this);
    ASSERT_INITIALIZED_VAL(input);
    return str_common_ends_with(this->ptr, this->count, input);
}

StrView StrView::view(sz start, sz offset) const
{
    if (offset == -1) offset = this->count;
    ASSERT_MSG(start + offset <= this->count, "Mustn't exceed count");
    return { this->ptr + start, offset };
}

StrView StrView::view_idx(sz start, sz end) const
{
    if (end == -1) end = this->count - 1;
    sz dist = (end - start) + 1;
    ASSERT_GREATER_ZERO(dist);
    ASSERT_MSG(start + dist <= this->count, "Mustn't exceed count");
    return { this->ptr + start, dist };
}

Maybe<sz> StrView::index_of(StrView seq) const
{
    return str_common_index_of(this->ptr, this->count, seq);
}

Maybe<sz> StrView::last_index_of(StrView seq) const
{
    return str_common_last_index_of(this->ptr, this->count, seq);
}

void StrView::trim_from_start_to_first_occur(char search, bool inclusive)
{
    return str_common_trim_from_start_to_first_occur(&this->ptr, &this->count, search, inclusive);
}

void StrView::trim_from_start_to_last_occur(char search, bool inclusive)
{
    return str_common_trim_from_start_to_last_occur(&this->ptr, &this->count, search, inclusive);
}

void StrView::trim_from_end_to_first_occur(char search, bool inclusive)
{
    return str_common_trim_from_end_to_first_occur(&this->ptr, &this->count, search, inclusive);
}

void StrView::trim_from_end_to_last_occur(char search, bool inclusive)
{
    return str_common_trim_from_end_to_last_occur(&this->ptr, &this->count, search, inclusive);
}

bool StrView::trim_sequence_start(StrView seq)
{
    return str_common_trim_sequence_start(&this->ptr, &this->count, seq);
}

bool StrView::trim_sequence_end(StrView seq)
{
    return str_common_trim_sequence_end(&this->ptr, &this->count, seq);
}

intern Maybe<sz> index_of_sv_128(StrView sv, char c);

Maybe<sz> StrView::index_of(char c) const
{
    ASSERT_NON_EMPTY(this);

#if RG_FEATURE_SIMD_128
    if (this->count < CHAR_LANE_COUNT_256) return index_of_sv_128(*this, c);
#endif

    Maybe<sz> res;
    const char* start = this->ptr;

#if defined(RG_FEATURE_SIMD_256)
    const __m256i search = _mm256_set1_epi8(c);
    __m256i seq;
    __m256i cmp_res;
    sz match_idx = 0;
    sz load_offset = 0;
    const sz count = this->count;
    u32 mask;
    bool found = false;

    while (this->count - load_offset >= CHAR_LANE_COUNT_256)
    {
        seq = _mm256_loadu_si256((const __m256i_u*)(start + load_offset));
        cmp_res = _mm256_cmpeq_epi8(seq, search);
        mask = (u32)_mm256_movemask_epi8(cmp_res);
        if (mask == 0) goto NEXT_ITER;

        match_idx += rg::ctz(mask);
        found = true;
        break;

    NEXT_ITER:
        match_idx += CHAR_LANE_COUNT_256;
        load_offset += CHAR_LANE_COUNT_256;
    }

    if (found)
    {
        res.set_val(match_idx);
        return res;
    }
#endif
    while (load_offset < count && start[load_offset] != c)
    {
        load_offset++;
    }

    if (load_offset >= count) return res;
    res.set_val(load_offset);
    return res;
}

#if RG_FEATURE_SIMD_128
intern Maybe<sz> index_of_sv_128(StrView sv, char c)
{
    Maybe<sz> res;
    const char* start = sv.ptr;

    const __m128i search = _mm_set1_epi8(c);
    __m128i seq;
    __m128i cmp_res;
    sz match_idx = 0;
    sz load_offset = 0;
    const sz count = sv.count;
    u16 mask;
    bool found = false;

    while (count - load_offset >= CHAR_LANE_COUNT_128)
    {
        seq = _mm_loadu_si128((const __m128i_u*)(start + load_offset));
        cmp_res = _mm_cmpeq_epi8(seq, search);
        mask = (u16)_mm_movemask_epi8(cmp_res);
        if (mask == 0) goto NEXT_ITER;

        match_idx += rg::ctz(mask);
        found = true;
        break;

    NEXT_ITER:
        match_idx += CHAR_LANE_COUNT_128;
        load_offset += CHAR_LANE_COUNT_128;
    }

    if (found)
    {
        res.set_val(match_idx);
        return res;
    }

    while (load_offset < count && start[load_offset] != c)
    {
        load_offset++;
    }

    if (load_offset >= count) return res;
    res.set_val(load_offset);
    return res;
}
#endif // RG_FEATURE_SIMD_128

#if RG_FEATURE_SIMD_128
intern Maybe<sz> last_index_of_sv_128(StrView sv, char c);
#endif

Maybe<sz> StrView::last_index_of(char c) const
{
    ASSERT_NON_EMPTY(this);

#if RG_FEATURE_SIMD_128
    if (this->count < CHAR_LANE_COUNT_256) return last_index_of_sv_128(*this, c);
#endif

    Maybe<sz> res;
    const char* start = this->ptr;
    const char* end = this->last_ref();

#if defined(RG_FEATURE_SIMD_256)
    const __m256i search = _mm256_set1_epi8(c);
    __m256i seq;
    __m256i cmp_res;
    sz match_idx = this->count - 1;
    sz load_offset = 0;
    const sz count = this->count;
    u32 mask;
    bool found = false;

    while (count - load_offset >= CHAR_LANE_COUNT_256)
    {
        seq = _mm256_loadu_si256((const __m256i_u*)(end - load_offset - CHAR_LANE_COUNT_256));
        cmp_res = _mm256_cmpeq_epi8(seq, search);
        mask = (u32)_mm256_movemask_epi8(cmp_res);
        if (mask == 0) goto NEXT_ITER;

        match_idx -= rg::clz(mask);
        found = true;
        break;

    NEXT_ITER:
        match_idx -= CHAR_LANE_COUNT_256;
        load_offset += CHAR_LANE_COUNT_256;
    }

    if (found)
    {
        res.set_val(match_idx);
        return res;
    }
#endif
    if (load_offset == 0) load_offset = count - 1;

    while (load_offset >= 0 && start[load_offset] != c)
    {
        load_offset--;
    }

    if (load_offset < 0) return res;
    res.set_val(load_offset);
    return res;
}

#if RG_FEATURE_SIMD_128
Maybe<sz> last_index_of_sv_128(StrView sv, char c)
{
    Maybe<sz> res;
    const char* start = sv.ptr;
    const char* end = sv.last_ref();

    const __m128i search = _mm_set1_epi8(c);
    __m128i seq;
    __m128i cmp_res;
    sz match_idx = sv.count - 1;
    sz load_offset = 0;
    const sz count = sv.count;
    u16 mask;
    bool found = false;

    while (count - load_offset >= CHAR_LANE_COUNT_128)
    {
        seq = _mm_loadu_si128((const __m128i_u*)(end - load_offset - CHAR_LANE_COUNT_128));
        cmp_res = _mm_cmpeq_epi8(seq, search);
        mask = (u16)_mm_movemask_epi8(cmp_res);
        if (mask == 0) goto NEXT_ITER;

        match_idx -= rg::clz(mask);
        found = true;
        break;

    NEXT_ITER:
        match_idx -= CHAR_LANE_COUNT_128;
        load_offset += CHAR_LANE_COUNT_128;
    }

    if (found)
    {
        res.set_val(match_idx);
        return res;
    }

    if (load_offset == 0) load_offset = count - 1;

    while (load_offset >= 0 && start[load_offset] != c)
    {
        load_offset--;
    }

    if (load_offset < 0) return res;
    res.set_val(load_offset);
    return res;
}
#endif // RG_FEATURE_SIMD_128

StrView StrView::slice_until_char(char c, bool inclusive)
{
    auto [idx, found] = this->index_of(c);
    if (!found) return this->view();
    if (inclusive && idx < this->count - 1) idx++;
    return this->view_idx(0, idx);
}

void StrView::trim_until_char(char c, bool inclusive)
{
    auto [idx, found] = this->index_of(c);
    if (!found) return;
    if (inclusive && idx < this->count - 1) idx++;
    this->ptr += idx;
    this->count -= idx;
}

StrView StrView::slice_while_callback(bool(*cb)(char))
{
    ASSERT(this->count > 0);

    const char* start = this->ptr;
    const char* curr = start;
    const char* end = this->end();

    while (curr != end && cb(*curr))
    {
        ++curr;
    }
    return StrView{ start, curr - start };
}

StrView StrView::slice_while_callback_and_trim(bool(*cb)(char))
{
    StrView res = this->slice_while_callback(cb);
    this->ptr += res.count;
    this->count -= res.count;
    return res;
}

StrView StrView::slice_until_callback(bool(*cb)(char))
{
    ASSERT(this->count > 0);

    const char* start = this->ptr;
    const char* curr = start;
    const char* end = this->end();

    while (curr != end && !cb(*curr))
    {
        ++curr;
    }
    return StrView{ start, curr - start };
}

StrView StrView::slice_until_callback_and_trim(bool(*cb)(char))
{
    StrView res = this->slice_until_callback(cb);
    this->ptr += res.count;
    this->count -= res.count;
    return res;
}

void StrView::trim_space_start()
{
    this->skip_chars_threshold_start(MAX_SPACE_CHAR);
}

void StrView::skip_chars_threshold_start(char threshold)
{
    const char* start = this->ptr;
    const char* curr = start;
    const char* end = this->end();

#if RG_FEATURE_SIMD_128
    __m128i lhs;
    const __m128i rhs = _mm_set1_epi8(threshold);
    __m128i cmp_res;
    sz first_non_space_idx = 0;
    sz load_offset = 0;
    u16 mask;
    bool found = false;

    while (this->count - load_offset >= CHAR_LANE_COUNT_128)
    {
        lhs = _mm_loadu_si128((const __m128i_u*)(start + load_offset));
        cmp_res = _mm_cmpgt_epi8(lhs, rhs);
        mask = (u16)_mm_movemask_epi8(cmp_res);

        if (mask == 0) goto NEXT_ITER;

        first_non_space_idx += rg::ctz(mask);
        found = true;
        break;

    NEXT_ITER:
        first_non_space_idx += CHAR_LANE_COUNT_128;
        load_offset += CHAR_LANE_COUNT_128;
    }

    if (found)
    {
        this->ptr += first_non_space_idx;
        this->count -= first_non_space_idx;
        return;
    }

    curr += load_offset;
#endif
    while (curr != end && rg::is_space(*curr))
    {
        curr++;
    }

    sz dist = curr - start;
    if (dist)
    {
        this->ptr += dist;
        this->count -= dist;
    }
}

void StrView::trim_space_end()
{
    const char* start = this->last_ref();
    const char* curr = start;
    const char* end = this->ptr;
    constexpr char SPACE_THRESHOLD = ' ' + 1;

#if RG_FEATURE_SIMD_128
    __m128i lhs;
    const __m128i rhs = _mm_set1_epi8(SPACE_THRESHOLD);
    __m128i cmp_res;
    sz first_non_space_idx = 0;
    sz load_offset = 0;
    u16 mask;
    s32 clz;
    bool found = false;

    while (this->count - load_offset >= CHAR_LANE_COUNT_128)
    {
        lhs = _mm_loadu_si128((const __m128i_u*)(start - load_offset - CHAR_LANE_COUNT_128));
        cmp_res = _mm_cmpgt_epi8(lhs, rhs);
        mask = (u16)_mm_movemask_epi8(cmp_res);

        if (mask == 0) goto NEXT_ITER;

        clz = rg::clz(mask);

        first_non_space_idx += clz;
        found = true;
        break;

    NEXT_ITER:
        first_non_space_idx += CHAR_LANE_COUNT_128;
        load_offset += CHAR_LANE_COUNT_128;
    }

    if (found)
    {
        this->count -= first_non_space_idx;
        return;
    }

    curr += load_offset;
#endif
    while (curr != end && rg::is_space(*curr))
    {
        curr--;
    }

    sz dist = curr - start;
    if (dist)
    {
        this->count -= dist;
    }
}

void StrView::trim_space_both()
{
    this->trim_space_start(); 
    this->trim_space_end(); 
}

bool contains_non_ascii(const char* start, const char* end)
{
    for (; start != end; ++start)
    {
        if (*start > 0x7F) return true;
    }
    return false;
}

// DString.

void DString::init_cstr(Allocator* alloc, CString cstr, bool preserve_null_term)
{
    sz len = ::strlen(cstr);
    sz init_cap = rg::max(DEFAULT_CAPACITY, len + 1);
    this->data = (char*)allocator_allocate(alloc, init_cap * sizeof(char));
    if (preserve_null_term) len += 1;
    this->count = len;
    this->capacity = init_cap;
    this->alloc = alloc;
    mem_copy(this->data, (void*)cstr, len);
}

void DString::init_view(Allocator* alloc, StrView str_view, sz additional_capacity)
{
    sz init_cap = rg::max(DEFAULT_CAPACITY, str_view.count + additional_capacity);
    this->data = (char*)allocator_allocate(alloc, init_cap * sizeof(char));
    this->count = 0;
    this->capacity = init_cap;
    this->alloc = alloc;
    if (str_view.count)
    {
        this->push(str_view);
    }
}

void DString::push(StrView sv)
{
    ASSERT_MSG(this->is_initialized(), "Must be initialized first");
    ASSERT_MSG(sv.is_initialized(), "Must be valid string view");

    // Remove null redundant null char.
    if (sv.is_null_term() && this->is_null_term()) this->count--;

    this->reserve(sv.count);
    char* copy_start = this->end();
    mem_copy(copy_start, (void*)sv.ptr, sv.count);
    this->count += sv.count;
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
        auto* talloc = get_temp_allocator();
        TEMP_ALLOC_SCOPE(talloc);
        char* res = (char*)allocator_allocate(talloc, size + 1);
        vsnprintf(res, size, fmt, args);
        va_start(args, fmt);
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

void DString::trim_end_n(sz trim_count)
{
    ASSERT_MSG(trim_count < this->count, "Shouldn't exceed inner count");
    common_trim_end_n((const char**)&this->data, &this->count, trim_count);
}

bool DString::starts_with(StrView input) const
{
    return str_common_starts_with(this->data, this->count, input);
}

bool DString::ends_with(StrView input) const
{
    return str_common_ends_with(this->data, this->count, input);
}

void DString::trim_from_end_to_first_occur(char search, bool inclusive)
{
    return str_common_trim_from_end_to_first_occur((const char**)&this->data, &this->count, search, inclusive);
}

void DString::trim_from_end_to_last_occur(char search, bool inclusive)
{
    return str_common_trim_from_end_to_last_occur((const char**)&this->data, &this->count, search, inclusive);
}

void DString::replace(char find, char replace)
{
    for (char& curr : *this)
    {
        if (curr == find) curr = replace;
    }
}

StrView DString::view(sz start, sz offset) const
{
    if (offset == -1) offset = this->count;
    ASSERT_MSG(start + offset <= this->count, "Mustn't exceed count");
    return { this->data + start, offset };
}

StrView DString::view_idx(sz start, sz end) const
{
    if (end == -1) end = this->count - 1;
    sz dist = (end - start) + 1;
    ASSERT_GREATER_ZERO(dist);
    ASSERT_MSG(start + dist <= this->count, "Mustn't exceed count");
    return { this->data + start, dist };
}

void DString::foreach_codepoint(void(*fn)(Utf8Codepoint&)) const
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

Utf8CodepointIterator DString::get_codepoint_iter() const
{
    StrView view = this->view();
    Utf8CodepointIterator iter; 
    iter.view = view;
    iter.pos = 0;
    return iter;
}

u64 DString::hash() const
{
    return rg::hash_fnv(this->data, this->count);
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

// Common code.

bool str_common_starts_with(const char* RESTRICT ptr, sz count, StrView seq)
{
    if (seq.count > count) return false;
    return mem_compare((void*)ptr, (void*)seq.ptr, seq.byte_size());
}

bool str_common_ends_with(const char* RESTRICT ptr, sz count, StrView input)
{
    if (input.count > count) return false;
    const char* start = ptr + (count - input.count);
    return mem_compare(start, input.ptr, input.byte_size());
}

bool str_common_trim_sequence_start(const char** RESTRICT ptr, sz* count, StrView trim_seq)
{
    if (!str_common_starts_with(*ptr, *count, trim_seq)) return false;
    *ptr += trim_seq.count;
    *count -= trim_seq.count;
    return true;
}

bool str_common_trim_sequence_end(const char** RESTRICT ptr, sz* count, StrView trim_seq)
{
    if (!str_common_ends_with(*ptr, *count, trim_seq)) return false;
    *count -= trim_seq.count;
    return true;
}

void str_common_trim_from_start_to_first_occur(const char** RESTRICT start, sz* count, char search, bool inclusive)
{
    auto [idx, found] = common_index_of(*start, *count, search);
    if (!found || idx == 0) return;
    if (!inclusive) idx++;
    *start += idx;
    *count -= idx;
}

void str_common_trim_from_start_to_last_occur(const char** RESTRICT start, sz* count, char search, bool inclusive)
{
    auto [idx, found] = common_last_index_of(*start, *count, search);
    if (!found || idx == 0) return;
    if (!inclusive) idx++;
    *start += idx;
    *count -= idx;
}

void str_common_trim_from_end_to_first_occur(const char** RESTRICT start, sz* count, char search, bool inclusive)
{
    auto [idx, found] = common_index_of(*start, *count, search);
    if (!found || idx == 0) return;
    if (inclusive) idx++;
    *count = idx;
}

void str_common_trim_from_end_to_last_occur(const char** RESTRICT start, sz* count, char search, bool inclusive)
{
    auto [idx, found] = common_index_of(*start, *count, search);
    if (!found || idx == 0) return;
    if (inclusive) idx++;
    *count = idx;
}

Maybe<sz> str_common_index_of(const char* RESTRICT start, sz count, StrView seq)
{
    if (seq.count == 1) return common_index_of(start, count, seq[0]);

    Maybe<sz> res;
    const char* curr;
    const char* inp_curr = seq.at_ref(1);
    const char match_start = seq[0];
    const char* end = start + count;
    const char* inp_end = seq.end();
    sz i = 0;

    for (; i < count && (count - i) >= seq.count; ++i)
    {
        if (start[i] == match_start)
        {
            curr = start + (i + 1);
            while (inp_curr != inp_end && curr != end && *curr == *inp_curr)
            {
                ++curr;
                ++inp_curr;
            }
            // Test for success.
            if (inp_curr == inp_end)
            {
                res.set_val(i);
                return res;
            }
            inp_curr = seq.at_ref(1);
        }
    }
    return res;
}

Maybe<sz> str_common_last_index_of(const char* RESTRICT start, sz count, StrView seq)
{
    if (seq.count == 1) return common_last_index_of(start, count, seq[0]);

    Maybe<sz> res;
    char match_start = seq.last();
    const char* curr;
    const char* inp_curr = seq.last_ref() - 1;
    const char* begin = start - 1;
    const char* inp_begin = seq.begin() - 1;
    sz i = count - 1;
    sz j;

    for (; i >= 0 && (i+1) >= seq.count; --i)
    {
        if (start[i] == match_start)
        {
            j = i;
            curr = start + (j - 1);
            while (inp_curr != inp_begin && curr != begin && *curr == *inp_curr)
            {
                --curr;
                --inp_curr;
                --j;
            }
            // Test for success.
            if (inp_curr == inp_begin)
            {
                res.set_val(j + 1);
                return res;
            }
            inp_curr = seq.last_ref() - 1;
        }
    }
    return res;
}

} // rg
