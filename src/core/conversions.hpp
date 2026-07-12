#include <math.h>
#include <stdio.h>
#include "collections/slice.hpp"
#include "collections/string.hpp"

namespace rg
{

template<typename IntType>
IntType string_to_int(Slice<char> str)
{
    trim_space_both((const char**)&str.ptr, &str.count);
    IntType res = 0;
    s32 sign = 1;
    char* curr = str.ptr;
    char* end = str.end();

    if (*curr == '-' || *curr == '+')
    {
        if (*curr == '-') sign = -1;
        curr++;
    }

    ASSERT_MSG(curr != end, "Should be digits after sign in a number");
    ASSERT_MSG(is_digit(*curr), "Must be a digit after space or sign");

    while (curr != end && is_digit(*curr))
    {
        res = res * 10 + (*curr - '0');
        ++curr;
    }

    return res * sign;
}

template<typename UintType>
UintType string_to_uint(Slice<char> str)
{
    trim_space_both((const char**)&str.ptr, &str.count);
    UintType res = 0;
    char* curr = str.ptr;
    char* end = str.end();

    ASSERT_MSG(*curr != '-' || *curr != '+', "Can't have a sign in a unsigned number conversion");
    ASSERT_MSG(curr != end, "Should be digits after sign in a number");
    ASSERT_MSG(is_digit(*curr), "Must be a digit after space or sign");

    while (curr != end && is_digit(*curr))
    {
        res = res * 10 + (*curr - '0');
        ++curr;
    }

    return res;
}

template<typename FloatType>
FloatType string_to_float(Slice<char> str)
{
    trim_space_both((const char**)&str.ptr, &str.count);
    FloatType res = 0.0;
    FloatType sign = 1.0;
    char* curr = str.ptr;
    char* end = str.end();
    
    if (*curr == '-' || *curr == '+')
    {
        if (*curr == '-') sign = -1;
        curr++;
    }

    ASSERT_MSG(curr != end, "Should be digits after sign in a number");
    ASSERT_MSG(is_digit(*curr), "Must be a digit after space / sign");

    while (curr != end && is_digit(*curr))
    {
        res = res * 10 + (*curr - '0');
        ++curr;
    }

    // Integer part.
    while (curr != end && is_digit(*curr))
    {
        res = res * 10.0 + (*curr - '0');
        ++curr;
    }

    ASSERT_MSG(curr != end && *curr == '.', "Must be dot after integral part of floating point number");
    ++curr;
    ASSERT_MSG(curr != end && is_digit(*curr), "Must be digits after dot in a floating point number");

    // Fractional part.
    FloatType factor = 0.1; 
    
    while (curr != end && is_digit(*curr))
    {
        res += (*curr - '0') * factor;
        factor *= 0.1;
        ++curr;
    }

    // Scientific notation
    if (curr != end && (*curr == 'e' || *curr == 'E'))
    {
        curr++;
        s32 sign = 1;
        if (curr != end && (*curr == '+' || *curr == '-'))
        {
            if (*curr == '-') {
                sign = -1;
            }
            curr++;
        }

        s32 exponent = 0;
        while (curr != end && is_digit(*curr))
        {
            exponent = exponent * 10 + (*curr - '0');
            curr++;
        }

        res *= pow(10.0, sign * exponent);
    }

    return res * sign;
}

template<typename IntType>
Slice<char> int_to_string(IntType int_val, char* out_buff, sz buff_len)
{
    s32 written = snprintf(out_buff, buff_len, "%dl", int_val); 
    return { out_buff, written };
}

template<typename UintType>
Slice<char> uint_to_string(UintType uint_val, char* out_buff, sz buff_len)
{
    s32 written = snprintf(out_buff, buff_len, "%ul", uint_val); 
    return { out_buff, written };
}

template<typename FloatType>
Slice<char> float_to_string(FloatType float_val, char* out_buff, sz buff_len)
{
    s32 written = snprintf(out_buff, buff_len, "%f", float_val); 
    return { out_buff, written };
}

} // rg
