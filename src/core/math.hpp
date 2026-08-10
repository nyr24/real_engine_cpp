#ifndef _RG_MATH_HPP_
#define _RG_MATH_HPP_

#include "core/basic.hpp"
#include "collections/farray.hpp"
#ifdef RG_FEATURE_SIMD_128
#include <immintrin.h>
#endif

namespace rg
{

// Utility.

template<typename Type>
constexpr Type ceil(Type val)
{
    return (Type)__builtin_ceil((f64)val);
}

template<typename Type>
constexpr Type floor(Type val)
{
    return (Type)__builtin_floor((f64)val);
}

template<typename Type>
constexpr Type sin(Type val)
{
    return (Type)__builtin_sin((f64)val);
}

template<typename Type>
constexpr Type cos(Type val)
{
    return (Type)__builtin_cos((f64)val);
}

template<typename Type>
constexpr Type tan(Type val)
{
    return (Type)__builtin_tan((f64)val);
}

template<typename Type>
constexpr Type asin(Type val)
{
    return (Type)__builtin_asin((f64)val);
}

template<typename Type>
constexpr Type acos(Type val)
{
    return (Type)__builtin_acos((f64)val);
}

template<typename Type>
constexpr Type atan(Type a, Type b)
{
    return (Type)__builtin_atan((f64)a);
}

template<typename Type>
constexpr Type atan2(Type a, Type b)
{
    return (Type)__builtin_atan2((f64)a, (f64)b);
}

template<typename Type>
constexpr Type sqrt(Type a)
{
    return (Type)__builtin_sqrt((f64)a);
}

template<typename Type>
constexpr Type pow(Type val, Type power)
{
    return (Type)__builtin_pow((f64)val, (f64)power);
}

template<typename Type>
constexpr Type ctz(Type val)
{
    return (Type)__builtin_ctz((u32)val);
}

template<>
constexpr u64 ctz(u64 val)
{
    return (u64)__builtin_ctzll(val);
}

template<typename Type>
constexpr Type clz(Type val)
{
    return (Type)__builtin_clz((u32)val);
}

template<>
constexpr u64 clz(u64 val)
{
    return (u64)__builtin_clzll((u32)val);
}

constexpr f64 PI = 3.14159265359;
constexpr f32 DEG_TO_RAD = PI / 180;
constexpr f32 RAD_TO_DEG = 180 / PI;

template<typename Type>
constexpr Type deg_to_rad(Type val)
{
    return val * DEG_TO_RAD;
}

template<typename Type>
constexpr Type rad_to_deg(Type val)
{
    return val * RAD_TO_DEG;
}

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

    void add_inplace(f32 rhs);
    void sub_inplace(f32 rhs);
    void div_inplace(f32 rhs);
    void mul_inplace(f32 rhs);

    Vec2 negate();
    Vec2 normalize();
    void negate_inplace();
    void normalize_inplace();
    f32 magnitude();
    f32 magnitude_squared();
    void print(LogLevel level = LogLevel::DEBUG);

    Vec2 operator-() { Vec2 res(*this); res.negate(); return res; }
    void operator+=(Vec2 b) { this->add_inplace(b); }
    void operator-=(Vec2 b) { this->sub_inplace(b); }
    void operator*=(Vec2 b) { this->mul_inplace(b); }
    void operator/=(Vec2 b) { this->div_inplace(b); }

    void operator+=(f32 b) { this->add_inplace(b); }
    void operator-=(f32 b) { this->sub_inplace(b); }
    void operator*=(f32 b) { this->mul_inplace(b); }
    void operator/=(f32 b) { this->div_inplace(b); }
};

Vec2 vec_add(Vec2 a, Vec2 b);
Vec2 vec_sub(Vec2 a, Vec2 b);
Vec2 vec_mul(Vec2 a, Vec2 b);
Vec2 vec_div(Vec2 a, Vec2 b);

Vec2 vec_add(Vec2 a, f32 b);
Vec2 vec_sub(Vec2 a, f32 b);
Vec2 vec_mul(Vec2 a, f32 b);
Vec2 vec_div(Vec2 a, f32 b);

Vec2 vec_direction(Vec2 a, Vec2 b);
f32 vec_dist(Vec2 a, Vec2 b);

Vec2 operator+(Vec2 a, Vec2 b);
Vec2 operator-(Vec2 a, Vec2 b);
Vec2 operator*(Vec2 a, Vec2 b);
Vec2 operator/(Vec2 a, Vec2 b);

Vec2 operator+(Vec2 a, f32 b);
Vec2 operator-(Vec2 a, f32 b);
Vec2 operator*(Vec2 a, f32 b);
Vec2 operator/(Vec2 a, f32 b);

Vec2 operator+(f32 a, Vec2 b);
Vec2 operator-(f32 a, Vec2 b);
Vec2 operator*(f32 a, Vec2 b);
Vec2 operator/(f32 a, Vec2 b);

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

    void add_inplace(f32 rhs);
    void sub_inplace(f32 rhs);
    void div_inplace(f32 rhs);
    void mul_inplace(f32 rhs);

    Vec3 negate();
    Vec3 normalize();
    void negate_inplace();
    void normalize_inplace();
    f32 magnitude();
    f32 magnitude_squared();
    void print(LogLevel level = LogLevel::DEBUG);

    Vec3 operator-() { Vec3 res(*this); res.negate(); return res; }

    void operator+=(Vec3 b) { this->add_inplace(b); }
    void operator-=(Vec3 b) { this->sub_inplace(b); }
    void operator*=(Vec3 b) { this->mul_inplace(b); }
    void operator/=(Vec3 b) { this->div_inplace(b); }

    void operator+=(f32 b) { this->add_inplace(b); }
    void operator-=(f32 b) { this->sub_inplace(b); }
    void operator*=(f32 b) { this->mul_inplace(b); }
    void operator/=(f32 b) { this->div_inplace(b); }
};

Vec3 vec_add(Vec3 a, Vec3 b);
Vec3 vec_sub(Vec3 a, Vec3 b);
Vec3 vec_mul(Vec3 a, Vec3 b);
Vec3 vec_div(Vec3 a, Vec3 b);

Vec3 vec_add(Vec3 a, f32 b);
Vec3 vec_sub(Vec3 a, f32 b);
Vec3 vec_mul(Vec3 a, f32 b);
Vec3 vec_div(Vec3 a, f32 b);

f32 vec_dot(Vec3 a, Vec3 b);
Vec3 vec_cross(Vec3 a, Vec3 b);

Vec3 vec_direction(Vec3 a, Vec3 b);
f32 vec_dist(Vec3 a, Vec3 b);

Vec3 operator+(Vec3 a, Vec3 b);
Vec3 operator-(Vec3 a, Vec3 b);
Vec3 operator*(Vec3 a, Vec3 b);
Vec3 operator/(Vec3 a, Vec3 b);

Vec3 operator+(Vec3 a, f32 b);
Vec3 operator-(Vec3 a, f32 b);
Vec3 operator*(Vec3 a, f32 b);
Vec3 operator/(Vec3 a, f32 b);

Vec3 operator+(f32 a, Vec3 b);
Vec3 operator-(f32 a, Vec3 b);
Vec3 operator*(f32 a, Vec3 b);
Vec3 operator/(f32 a, Vec3 b);

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

struct alignas(16) Vec4
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

    void add_inplace(f32 rhs);
    void sub_inplace(f32 rhs);
    void div_inplace(f32 rhs);
    void mul_inplace(f32 rhs);

    Vec4 negate();
    Vec4 normalize();
    void negate_inplace();
    void normalize_inplace();
    f32 magnitude();
    f32 magnitude_squared();
    void print(LogLevel level = LogLevel::DEBUG);

    Vec4 operator-() { Vec4 res(*this); res.negate(); return res; }

    void operator+=(Vec4 b) { this->add_inplace(b); }
    void operator-=(Vec4 b) { this->sub_inplace(b); }
    void operator*=(Vec4 b) { this->mul_inplace(b); }
    void operator/=(Vec4 b) { this->div_inplace(b); }

    void operator+=(f32 b) { this->add_inplace(b); }
    void operator-=(f32 b) { this->sub_inplace(b); }
    void operator*=(f32 b) { this->mul_inplace(b); }
    void operator/=(f32 b) { this->div_inplace(b); }
};

Vec4 vec_add(Vec4 a, Vec4 b);
Vec4 vec_sub(Vec4 a, Vec4 b);
Vec4 vec_mul(Vec4 a, Vec4 b);
Vec4 vec_div(Vec4 a, Vec4 b);

Vec4 vec_add(Vec4 a, f32 b);
Vec4 vec_sub(Vec4 a, f32 b);
Vec4 vec_mul(Vec4 a, f32 b);
Vec4 vec_div(Vec4 a, f32 b);

f32 vec_dot(Vec4 a, Vec4 b);
Vec4 vec_cross(Vec4 a, Vec4 b);

Vec4 vec_direction(Vec4 a, Vec4 b);
f32 vec_dist(Vec4 a, Vec4 b);

Vec4 operator+(Vec4 a, Vec4 b);
Vec4 operator-(Vec4 a, Vec4 b);
Vec4 operator*(Vec4 a, Vec4 b);
Vec4 operator/(Vec4 a, Vec4 b);

Vec4 operator+(Vec4 a, f32 b);
Vec4 operator-(Vec4 a, f32 b);
Vec4 operator*(Vec4 a, f32 b);
Vec4 operator/(Vec4 a, f32 b);

Vec4 operator+(f32 a, Vec4 b);
Vec4 operator-(f32 a, Vec4 b);
Vec4 operator*(f32 a, Vec4 b);
Vec4 operator/(f32 a, Vec4 b);

// Quaternion.

enum RotationAxis
{
    X,
    Y,
    Z
};

const Array<RotationAxis, 3> DEFAULT_ROTATION_ORDER = { RotationAxis::X, RotationAxis::Y, RotationAxis::Z };

struct Mat4;

struct Quat : Vec4
{
    using Vec4::Vec4;

    static Quat identity()
    {
        return { 0, 0, 0, 1 };
    }
    static Quat create_angle_axis(f32 angle_deg, Vec3 axis);
    static Quat create_euler(f32 pitch_deg, f32 yaw_deg, f32 roll_deg);
    static Quat create_pitch(f32 pitch_deg);
    static Quat create_yaw(f32 yaw_deg);
    static Quat create_roll(f32 roll_deg);
    static Quat create_from_matrix(const Mat4& mat);

    Vec3 rotate_point(Vec3 point);
    void rotate(Vec3 angles_deg, Array<RotationAxis, 3> order);
    void rotate(f32 pitch_deg, f32 yaw_deg, f32 roll_deg, Array<RotationAxis, 3> order);
    void rotate_x(f32 pitch_deg);
    void rotate_y(f32 yaw_deg);
    void rotate_z(f32 roll_deg);
    Quat conjugate(); 
    Quat inverse();
    void inverse_inplace();
    Quat normalize();
    void normalize_inplace();

    Quat& operator*=(Quat rhs);
};

Quat operator*(Quat lhs, Quat rhs);

/*
 Matrix4x4.
 Stored and used in the shader as row-major.
 Multiplication order with vector: Vector * Matrix.
 Perspective: right-handed [0, 1].
*/

struct alignas(16) Mat4 : Array<f32, 16>
{
    using Array<f32, 16>::Array;

    static Mat4 identity();
    static Mat4 init_translate(Vec3 v);
    static Mat4 init_scale(Vec3 v);
    static Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up);
    static Mat4 ortho(f32 left, f32 right, f32 bottom, f32 top,  f32 z_near, f32 z_far);
    static Mat4 perspective(f32 fovy, f32 aspect, f32 z_near, f32 z_far);
    static Mat4 frustum(f32 left, f32 right, f32 bottom, f32 top, f32 z_near, f32 z_far);

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
    Vec3 extract_translation() const;
    Vec3 extract_scale() const;
    Quat extract_rotation() const;
    void print(LogLevel level = LogLevel::DEBUG);
};

Mat4 quat_to_matrix(Quat q);

Mat4 operator*(const Mat4& a, const Mat4& b);

// Simd.

#ifdef RG_FEATURE_SIMD_128

alias f128 = __m128;
alias s128 = __m128i;
alias d128 = __m128d;

f128 mul_f128(f128 a, f128 b);
f128 add_f128(f128 a, f128 b);
f128 sub_f128(f128 a, f128 b);
f128 div_f128(f128 a, f128 b);
f32 dot_f128(f128 a, f128 b);
f128 cross_f128(f128 a, f128 b);

#endif // RG_FEATURE_SIMD_128

#ifdef RG_FEATURE_SIMD_256

alias f256 = __m256;
alias s256 = __m256i;
alias d256 = __m256d;

f256 mul_f256(f256 a, f256 b);
f256 add_f256(f256 a, f256 b);
f256 sub_f256(f256 a, f256 b);
f256 div_f256(f256 a, f256 b);

#endif // RG_FEATURE_SIMD_256

} // rg

#endif // _RG_MATH_HPP_
