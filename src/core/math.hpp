#ifndef _RG_MATH_HPP_
#define _RG_MATH_HPP_

#include <emmintrin.h>
#include <immintrin.h>
#include <xmmintrin.h>
#include "core/basic.hpp"
#include "collections/farray.hpp"

namespace rg
{

alias f128 = __m128;
alias f256 = __m256;
alias f512 = __m512;

alias s128 = __m128i;
alias s256 = __m256i;
alias s512 = __m512i;

alias d128 = __m128d;
alias d256 = __m256d;
alias d512 = __m512d;

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

template<typename SimdType, typename Type, sz COUNT>
struct Vec128
{
    static_assert(sizeof(Type) * COUNT <= sizeof(f128), "Mustn't exceed the underlying type");

    // Probably UB, but its convenient.
    union
    {
        SimdType repr;
        Array<Type, COUNT> arr;
    };
};

struct Vec128char : Vec128<__m128, char, 16>
{
    Vec128char()
    {
        this->repr = _mm_setzero_si128();
    }
    Vec128char(char val)
    {
        this->repr = _mm_set1_epi8(val);
    }
    Vec128char(Slice<char> vals);
    static Vec128char create(Slice<char> vals) { return { vals }; }
    bool cmp(const Vec128char& rhs);
    Maybe<sz> find_first_match(const Vec128char& rhs);
};

bool operator==(const Vec128char& a, const Vec128char& b);

template<sz COUNT>
struct Vec128f : Vec128<__m128, f32, COUNT>
{
    Vec128f()
    {
        this->repr = _mm_setzero_ps();
    }
    Vec128f(f128 repr_)
    {
        this->repr = repr_;
    }

    static Vec128f zero() { return { _mm_setzero_ps() }; }
    static Vec128f one() { return { _mm_set1_ps(1.0f) }; }
    static Vec128f create(f32 val) { return { _mm_set1_ps(val) }; }
    inline f32 operator[](sz idx) { return this->repr[idx]; }
};

template<sz COUNT>
Vec128f<COUNT> operator*(Vec128f<COUNT> a, Vec128f<COUNT> b) { return { mul_f128(a.val, b.val) }; }
template<sz COUNT>
Vec128f<COUNT> operator+(Vec128f<COUNT> a, Vec128f<COUNT> b) { return { add_f128(a.val, b.val) }; }
template<sz COUNT>
Vec128f<COUNT> operator-(Vec128f<COUNT> a, Vec128f<COUNT> b) { return { sub_f128(a.val, b.val) }; }
template<sz COUNT>
Vec128f<COUNT> operator/(Vec128f<COUNT> a, Vec128f<COUNT> b) { return { div_f128(a.val, b.val) }; }

struct Vec2 : Vec128f<2>
{
    Vec2(f128 repr) { this->repr = repr; }
    Vec2(f32 x, f32 y) { this->repr = _mm_setr_ps(x, y, 0, 1.0f); }
    static Vec2 create(f32 x, f32 y) { return { x, y }; }
    f32 dot(Vec2 rhs) { return dot_f128(this->repr, rhs.repr); }
};

struct Vec3 : Vec128f<3>
{
    Vec3(f32 x, f32 y, f32 z) { this->repr = _mm_setr_ps(x, y, z, 0); }
    Vec3(f128 repr) { this->repr = repr; }
    static Vec3 create(f32 x, f32 y, f32 z) { return { x, y, z }; }
    f32 dot(Vec3 rhs) { return dot_f128(this->repr, rhs.repr); }
    Vec3 cross(Vec3 rhs) { return { cross_f128(this->repr, rhs.repr) }; }
};

struct Vec4 : Vec128f<4>
{
    Vec4(f32 x, f32 y, f32 z, f32 w) { this->repr = _mm_setr_ps(x, y, z, w); }
    Vec4(f128 repr) { this->repr = repr; }
    static Vec4 create(f32 x, f32 y, f32 z, f32 w) { return { x, y, z, w }; }
    f32 dot(Vec4 rhs) { return dot_f128(this->repr, rhs.repr); }
    Vec4 cross(Vec4 rhs) { return { cross_f128(this->repr, rhs.repr) }; }
};

// Matrix (squared).

template<sz ROWS>
struct Mat512f
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

struct Mat4 : Mat512f<4>
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
