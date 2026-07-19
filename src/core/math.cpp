#include "math.hpp"
#include <emmintrin.h>

namespace rg
{

// Vec.

// Vec128char.

Vec128char::Vec128char(Slice<char> vals)
{
    ASSERT_MSG(vals.count == 16, "Must be equal to 16 using Vec128char vector");
    this->repr = _mm_setr_epi8(
        vals[0], vals[1], vals[2], vals[3],
        vals[4], vals[5], vals[6], vals[7],
        vals[8], vals[9], vals[10], vals[11],
        vals[12], vals[13], vals[14], vals[15]
    );
}

bool Vec128char::cmp(const Vec128char& rhs)
{
    return *this == rhs;
}

Maybe<sz> Vec128char::find_first_match(const Vec128char& rhs)
{
    Maybe<sz> res;
    s128 cmp_res = _mm_cmpeq_epi8(this->repr, rhs.repr);
    s32 mask = _mm_movemask_epi8(cmp_res);
    s32 ctz = __builtin_ctz(mask);
    if (ctz == bitsizeof(s32)) return res;
    res.set_val((sz)ctz);
    return res;
}

bool operator==(const Vec128char& a, const Vec128char& b)
{
    s128 res = _mm_cmpeq_epi8(a.repr, b.repr);
    s32 mask = _mm_movemask_epi8(res);
    return __builtin_ctz(mask) == bitsizeof(s32);
}

// Matrix.

Mat4 Mat4::identity()
{
    return { _mm512_setr_ps(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    ) };
}

Mat4 Mat4::translation(Vec3 v)
{
    return { _mm512_setr_ps(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        v[0], v[1], v[2], 1
    ) };
}

Mat4 Mat4::scale(Vec3 v)
{
    return { _mm512_setr_ps(
        v[0], 0, 0, 0,
        0, v[1], 0, 0,
        0, 0, v[2], 0,
        0, 0, 0, 1
    ) };
}

__m512i TRANSPOSE_MASK_MAT4 = _mm512_set_epi32(
    15, 11, 7, 3,
    14, 10, 6, 2,
    13,  9, 5, 1,
    12,  8, 4, 0
);

Mat4 Mat4::transposed()
{
    return { _mm512_permutexvar_ps(TRANSPOSE_MASK_MAT4, this->repr) };
}

void Mat4::transpose()
{
    this->repr = _mm512_permutexvar_ps(TRANSPOSE_MASK_MAT4, this->repr);
}

Mat4& Mat4::operator*=(const Mat4& mat)
{
    *this = *this * mat;
    return *this;
}

Mat4 operator*(const Mat4& a, const Mat4& b)
{
    __m512 b_row0 = _mm512_shuffle_f32x4(b.repr, b.repr, _MM_SHUFFLE(0, 0, 0, 0));
    __m512 b_row1 = _mm512_shuffle_f32x4(b.repr, b.repr, _MM_SHUFFLE(1, 1, 1, 1));
    __m512 b_row2 = _mm512_shuffle_f32x4(b.repr, b.repr, _MM_SHUFFLE(2, 2, 2, 2));
    __m512 b_row3 = _mm512_shuffle_f32x4(b.repr, b.repr, _MM_SHUFFLE(3, 3, 3, 3));
    __m512 a_col0 = _mm512_shuffle_ps(a.repr, a.repr, _MM_SHUFFLE(0, 0, 0, 0));
    __m512 res = _mm512_mul_ps(a_col0, b_row0);
    __m512 a_col1 = _mm512_shuffle_ps(a.repr, a.repr, _MM_SHUFFLE(1, 1, 1, 1));
    res = _mm512_fmadd_ps(a_col1, b_row1, res);
    __m512 a_col2 = _mm512_shuffle_ps(a.repr, a.repr, _MM_SHUFFLE(2, 2, 2, 2));
    res = _mm512_fmadd_ps(a_col2, b_row2, res);
    __m512 a_col3 = _mm512_shuffle_ps(a.repr, a.repr, _MM_SHUFFLE(3, 3, 3, 3));
    res = _mm512_fmadd_ps(a_col3, b_row3, res);

    return Mat4{ res };
}

// Underlying operations.

// f128.

f128 mul_f128(f128 a, f128 b)
{
    return _mm_mul_ps(a, b);
}

f128 add_f128(f128 a, f128 b)
{
    return _mm_add_ps(a, b);
}

f128 sub_f128(f128 a, f128 b)
{
    return _mm_sub_ps(a, b);
}

f128 div_f128(f128 a, f128 b)
{
    return _mm_div_ps(a, b);
}

f32 dot_f128(f128 a, f128 b)
{
    // 1. Element-wise multiplication: [a3*b3, a2*b2, a1*b1, a0*b0]
    f128 prod = _mm_mul_ps(a, b); 
    
    // 2. Horizontal add step 1: combine pairs
    // [*, *, a3*b3 + a2*b2, a1*b1 + a0*b0]
    f128 shuf1 = _mm_shuffle_ps(prod, prod, _MM_SHUFFLE(2, 3, 0, 1));
    f128 sum1  = _mm_add_ps(prod, shuf1);
    
    // 3. Horizontal add step 2: combine the pairs into the final scalar sum
    // [*, *, *, total_sum]
    f128 shuf2 = _mm_shuffle_ps(sum1, sum1, _MM_SHUFFLE(1, 0, 3, 2));
    f128 final_sum = _mm_add_ps(sum1, shuf2);
    
    return _mm_cvtss_f32(final_sum);
}

#define PERMUTE_MASK_F128(x, y, z, w) ((w << 6) | (z << 4) | (y << 2) | x)

f128 cross_f128(f128 a, f128 b)
{
    f128 a_yzx = _mm_permute_ps(a, PERMUTE_MASK_F128(1, 2, 0, 3));
    f128 b_zxy = _mm_permute_ps(b, PERMUTE_MASK_F128(2, 0, 1, 3));
    f128 a_zxy = _mm_permute_ps(a, PERMUTE_MASK_F128(2, 0, 1, 3));
    f128 b_yzx = _mm_permute_ps(b, PERMUTE_MASK_F128(1, 2, 0, 3));

    f128 mul1 = _mm_mul_ps(a_yzx, b_zxy);
    f128 mul2 = _mm_mul_ps(a_zxy, b_yzx);
    
    return _mm_sub_ps(mul1, mul2);
}

// f256.

f256 mul_f256(f256 a, f256 b)
{
    return _mm256_mul_ps(a, b);
}

f256 add_f256(f256 a, f256 b)
{
    return _mm256_add_ps(a, b);
}

f256 sub_f256(f256 a, f256 b)
{
    return _mm256_sub_ps(a, b);
}

f256 div_f256(f256 a, f256 b)
{
    return _mm256_div_ps(a, b);
}

// f512.

f512 mul_f512(f512 a, f512 b)
{
    return _mm512_mul_ps(a, b);
}

f512 add_f512(f512 a, f512 b)
{
    return _mm512_add_ps(a, b);
}

f512 sub_f512(f512 a, f512 b)
{
    return _mm512_sub_ps(a, b);
}

f512 div_f512(f512 a, f512 b)
{
    return _mm512_div_ps(a, b);
}

} // rg
