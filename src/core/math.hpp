#ifndef _RG_MATH_HPP_
#define _RG_MATH_HPP_

#include <immintrin.h>
#include "core/basic.hpp"

namespace rg
{

alias f128 = __m128;
alias f256 = __m256;
alias f512 = __m512;

f128 mul_f128(f128 a, f128 b);
f128 add_f128(f128 a, f128 b);
f128 sub_f128(f128 a, f128 b);
f128 div_f128(f128 a, f128 b);
f32 dot_f128(f128 a, f128 b);
f128 cross_f128(f128 a, f128 b);
f256 mul_f256(f256 a, f256 b);
f256 add_f256(f256 a, f256 b);
f256 sub_f256(f256 a, f256 b);
f256 div_f256(f256 a, f256 b);
f512 mul_f512(f512 a, f512 b);
f512 add_f512(f512 a, f512 b);
f512 sub_f512(f512 a, f512 b);
f512 div_f512(f512 a, f512 b);

template<sz COUNT>
struct Vec128
{
    static_assert(COUNT >= 2 && COUNT <= 4, "Supported only 2-4 components for 128 bit vector");

    // Probably UB, but its convenient.
    union
    {
        f128 repr;
        f32 arr[COUNT];
    };

    inline void to_arr(f32 arr[COUNT]) { _mm_store_ps(arr, this->repr); }

    static Vec128 zero() { return Vec128{ _mm_setzero_ps() }; }
    static Vec128 unit() { return Vec128{ _mm_set1_ps(1.0f) }; }
    static Vec128 create(f32 val) { return Vec128{ _mm_set1_ps(val) }; }
    inline f32 operator[](sz idx) { return this->repr[idx]; }
};

template<sz COUNT>
Vec128<COUNT> operator*(Vec128<COUNT> a, Vec128<COUNT> b) { return { mul_f128(a.val, b.val) }; }
template<sz COUNT>
Vec128<COUNT> operator+(Vec128<COUNT> a, Vec128<COUNT> b) { return { add_f128(a.val, b.val) }; }
template<sz COUNT>
Vec128<COUNT> operator-(Vec128<COUNT> a, Vec128<COUNT> b) { return { sub_f128(a.val, b.val) }; }
template<sz COUNT>
Vec128<COUNT> operator/(Vec128<COUNT> a, Vec128<COUNT> b) { return { div_f128(a.val, b.val) }; }

struct Vec2 : Vec128<2>
{
    static Vec128 create(f32 x, f32 y) { return Vec128{ _mm_setr_ps(x, y, 0, 0) }; }
};

struct Vec3 : Vec128<3>
{
    Vec3(f32 x, f32 y, f32 z) { this->repr = _mm_setr_ps(x, y, z, 0); }
    Vec3(f128 repr) { this->repr = repr; }
    static Vec3 create(f32 x, f32 y, f32 z) { return { x, y, z }; }
    f32 dot(Vec3 rhs) { return dot_f128(this->repr, rhs.repr); }
    Vec3 cross(Vec3 rhs) { return { cross_f128(this->repr, rhs.repr) }; }
};

struct Vec4 : Vec128<4>
{
    Vec4(f32 x, f32 y, f32 z, f32 w) { this->repr = _mm_setr_ps(x, y, z, w); }
    Vec4(f128 repr) { this->repr = repr; }
    static Vec4 create(f32 x, f32 y, f32 z, f32 w) { return { x, y, z, w }; }
    f32 dot(Vec4 rhs) { return dot_f128(this->repr, rhs.repr); }
    Vec4 cross(Vec4 rhs) { return { cross_f128(this->repr, rhs.repr) }; }
};

// Matrix (squared).

template<sz ROWS>
struct Mat512
{
    static constexpr sz COLS = ROWS;
    static constexpr sz ELEM_COUNT = ROWS * COLS;
    static_assert(ELEM_COUNT >= 4 && ELEM_COUNT <= 16, "Supported matrices: 2x2, 3x3, 4x4");
    
    // Probably UB, but its convenient.
    union
    {
        f512 repr;
        f32 arr[ELEM_COUNT];
    };
};

struct Mat4 : Mat512<4>
{
    static Mat4 identity();
    static Mat4 translation(Vec3 vec);
    static Mat4 scale(Vec3 vec);
    // TODO: rotation
    // static Mat4 rotation(f32 angle_deg, f32 axis[3]);

    Mat4 transposed();
    void transpose();
    Mat4& operator*=(const Mat4&);
};

Mat4 operator*(const Mat4& a, const Mat4& b);

} // rg

#endif // _RG_MATH_HPP_
