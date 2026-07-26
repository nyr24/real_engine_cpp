#ifndef _RG_MATH_HPP_
#define _RG_MATH_HPP_

#include "core/basic.hpp"
#include "collections/farray.hpp"
#include <immintrin.h>

namespace rg
{

// Vec2.

struct Vec2
{
    union
    {
        f32 arr[2];
        struct
        {
            f32 x;
            f32 y;
        };
    };

    Vec2() = default;
    Vec2(f32 x, f32 y): x{x}, y{y} {}

    static Vec2 create(f32 x, f32 y) { return { x, y }; }
    static Vec2 create_repeat(f32 r) { return { r, r }; }
    static Vec2 zero() { return { 0, 0 }; }
    static Vec2 one() { return { 1, 1 }; }

    void add_inplace(Vec2 rhs);
    void sub_inplace(Vec2 rhs);
    void div_inplace(Vec2 rhs);
    void mul_inplace(Vec2 rhs);
    void negate();
    void normalize();
    f32 magninute();
    f32 magninute_square();

    Vec2 operator-() { Vec2 res(*this); res.negate(); return res; }
    void operator+=(Vec2 b) { this->add_inplace(b); }
    void operator-=(Vec2 b) { this->sub_inplace(b); }
    void operator*=(Vec2 b) { this->mul_inplace(b); }
    void operator/=(Vec2 b) { this->div_inplace(b); }
};

Vec2 vec3_add(Vec2 a, Vec2 b);
Vec2 vec3_sub(Vec2 a, Vec2 b);
Vec2 vec3_mul(Vec2 a, Vec2 b);
Vec2 vec3_div(Vec2 a, Vec2 b);
f32 vec3_dot(Vec2 a, Vec2 b);
Vec2 vec3_cross(Vec2 a, Vec2 b);

Vec2 vec2_direction(Vec2 a, Vec2 b);
f32 vec2_dist(Vec2 a, Vec2 b);

Vec2 operator+(Vec2 a, Vec2 b);
Vec2 operator-(Vec2 a, Vec2 b);
Vec2 operator*(Vec2 a, Vec2 b);
Vec2 operator/(Vec2 a, Vec2 b);

// Vec3.

struct BaseVec3
{
    union
    {
        f32 arr[3];
        struct
        {
            f32 x;
            f32 y;
            f32 z;
        };
        struct
        {
            f32 r;
            f32 g;
            f32 b;
        };
    };

    BaseVec3() = default;
    BaseVec3(f32 x, f32 y, f32 z) : x{x}, y{y}, z{z} {}

    static BaseVec3 create(f32 x, f32 y, f32 z) { return { x, y, z }; }
    static BaseVec3 create_repeat(f32 r) { return { r, r, r }; }
    static BaseVec3 zero() { return { 0, 0, 0 }; }
    static BaseVec3 one() { return { 1, 1, 1 }; }
};

#ifdef RG_FEATURE_SIMD_128
struct alignas(16) Vec3
#else
struct Vec3
#endif
{
    union
    {
        f32 arr[3];
        struct
        {
            f32 x;
            f32 y;
            f32 z;
        };
        struct
        {
            f32 r;
            f32 g;
            f32 b;
        };
    };

    Vec3() = default;
    Vec3(f32 x, f32 y, f32 z) : x{x}, y{y}, z{z} {}

    static Vec3 create(f32 x, f32 y, f32 z) { return { x, y, z }; }
    static Vec3 create_repeat(f32 r) { return { r, r, r }; }
    static Vec3 zero() { return { 0, 0, 0 }; }
    static Vec3 one() { return { 1, 1, 1 }; }

    void add_inplace(Vec3 rhs);
    void sub_inplace(Vec3 rhs);
    void div_inplace(Vec3 rhs);
    void mul_inplace(Vec3 rhs);
    void negate();
    void normalize();
    f32 magninute();
    f32 magninute_square();

    Vec3 operator-() { Vec3 res(*this); res.negate(); return res; }
    void operator+=(Vec3 b) { this->add_inplace(b); }
    void operator-=(Vec3 b) { this->sub_inplace(b); }
    void operator*=(Vec3 b) { this->mul_inplace(b); }
    void operator/=(Vec3 b) { this->div_inplace(b); }
};

Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
Vec3 vec3_mul(Vec3 a, Vec3 b);
Vec3 vec3_div(Vec3 a, Vec3 b);
f32 vec3_dot(Vec3 a, Vec3 b);
Vec3 vec3_cross(Vec3 a, Vec3 b);

Vec3 vec3_direction(Vec3 a, Vec3 b);
f32 vec3_dist(Vec3 a, Vec3 b);

Vec3 operator+(Vec3 a, Vec3 b);
Vec3 operator-(Vec3 a, Vec3 b);
Vec3 operator*(Vec3 a, Vec3 b);
Vec3 operator/(Vec3 a, Vec3 b);

// Vec4. (W component is used for homogenous coordinates, and will be always 1 and intouched in math operations).

// For storing colors or other read-only data.
struct BaseVec4
{
    union
    {
        f32 arr[4];
        struct
        {
            f32 x;
            f32 y;
            f32 z;
            f32 w;
        };
        struct
        {
            f32 r;
            f32 g;
            f32 b;
            f32 a;
        };
    };

    BaseVec4() = default;
    BaseVec4(f32 x, f32 y, f32 z, f32 w) : x{x}, y{y}, z{z}, w{w} {}

    static BaseVec4 create(f32 x, f32 y, f32 z, f32 w = 1) { return { x, y, z, w }; }
    static BaseVec4 create_repeat(f32 r) { return { r, r, r, r }; }
    static BaseVec4 zero() { return { 0, 0, 0, 0 }; }
    static BaseVec4 one() { return { 1, 1, 1, 1 }; }
 };

struct alignas(16) Vec4 : BaseVec4
{
    union
    {
        f32 arr[4];
        struct
        {
            f32 x;
            f32 y;
            f32 z;
            f32 w;
        };
        struct
        {
            f32 r;
            f32 g;
            f32 b;
            f32 a;
        };
    };

    Vec4() = default;
    Vec4(f32 x, f32 y, f32 z, f32 w) : x{x}, y{y}, z{z}, w{w} {}

    static Vec4 create(f32 x, f32 y, f32 z, f32 w = 1) { return { x, y, z, w }; }
    static Vec4 create_repeat(f32 r) { return { r, r, r, r }; }
    static Vec4 create_repeat_homogenous(f32 r) { return { r, r, r, 1 }; }
    static Vec4 zero() { return { 0, 0, 0, 0 }; }
    static Vec4 zero_homogenous() { return { 0, 0, 0, 1 }; }
    static Vec4 one() { return { 1, 1, 1, 1 }; }
 
    void add_inplace(Vec4 rhs);
    void sub_inplace(Vec4 rhs);
    void div_inplace(Vec4 rhs);
    void mul_inplace(Vec4 rhs);
    void negate();
    void normalize();
    f32 magninute();
    f32 magninute_square();

    Vec4 operator-() { Vec4 res(*this); res.negate(); return res; }
    void operator+=(Vec4 b) { this->add_inplace(b); }
    void operator-=(Vec4 b) { this->sub_inplace(b); }
    void operator*=(Vec4 b) { this->mul_inplace(b); }
    void operator/=(Vec4 b) { this->div_inplace(b); }
};

Vec4 vec4_add(Vec4 a, Vec4 b);
Vec4 vec4_sub(Vec4 a, Vec4 b);
Vec4 vec4_mul(Vec4 a, Vec4 b);
Vec4 vec4_div(Vec4 a, Vec4 b);
f32 vec4_dot(Vec4 a, Vec4 b);
Vec4 vec4_cross(Vec4 a, Vec4 b);

Vec4 vec4_direction(Vec4 a, Vec4 b);
f32 vec4_dist(Vec4 a, Vec4 b);

Vec4 operator+(Vec4 a, Vec4 b);
Vec4 operator-(Vec4 a, Vec4 b);
Vec4 operator*(Vec4 a, Vec4 b);
Vec4 operator/(Vec4 a, Vec4 b);

// Matrix4.
// Multiplication order with vector: Matrix * Vector. (Row-major)
// Implements right-handed [0, 1] perspective.

struct alignas(16) Mat4 : Array<f32, 16>
{
    using Array<f32, 16>::Array;

    static Mat4 identity();
    static Mat4 init_translate(Vec3 v);
    static Mat4 init_scale(Vec3 v);
    static Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up);
    static Mat4 ortho(f32 left, f32 right, f32 bottom, f32 top,  f32 z_near, f32 z_far);
    static Mat4 proj(f32 fovy, f32 aspect, f32 z_near, f32 z_far);

    Mat4 translate(Vec3 v) const;
    Mat4 scale(Vec3 v) const;
    Mat4 transpose() const;

    void translate_inplace(Vec3 v);
    void scale_inplace(Vec3 v);
    void mul_inplace(const Mat4& rhs);
    void transpose_inplace();
    void invert_affine();
    void invert_general();

    // If the matrix is orthogonal (90deg between axis, and all axis are normalized)
    // As in camera matrix.
    void invert_orhogonal() { this->transpose_inplace(); }
    void operator*=(const Mat4& rhs) { this->mul_inplace(rhs); }
};

Mat4 operator*(Mat4 a, Mat4 b);

// Quaternion.

struct Quat : Vec4
{
    using Vec4::Vec4;

    static Quat create(f32 x, f32 y, f32 z, f32 w)
    {
        return { x, y, z, w }; 
    }
    static Quat zero()
    {
        return { 0, 0, 0, 0 };
    }
    static Quat one()
    {
        return { 1, 1, 1, 1 };
    }

    static Quat create_from_euler(f32 pitch, f32 yaw, f32 roll);
    Quat conjugate() const; 
    Mat4 to_matrix() const;
};

// Simd.

alias f128 = __m128;
alias f256 = __m256;

alias s128 = __m128i;
alias s256 = __m256i;

alias d128 = __m128d;
alias d256 = __m256d;

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

} // rg

#endif // _RG_MATH_HPP_
