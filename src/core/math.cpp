#include "core/basic.hpp"
#include "core/math.hpp"
#include "math.h"

namespace rg
{

// Vec2.

Vec2 vec2_add(Vec2 lhs, Vec2 rhs)
{
    return { lhs.x + rhs.x, lhs.y + rhs.y };
}

Vec2 vec2_sub(Vec2 lhs, Vec2 rhs)
{
    return { lhs.x - rhs.x, lhs.y - rhs.y };
}

Vec2 vec2_mul(Vec2 lhs, Vec2 rhs)
{
    return { lhs.x * rhs.x, lhs.y * rhs.y };
}

Vec2 vec2_div(Vec2 lhs, Vec2 rhs)
{
    return { lhs.x / rhs.x, lhs.y / rhs.y };
}

void Vec2::add_inplace(Vec2 rhs)
{
    this->x += rhs.x;
    this->y += rhs.y;
}

void Vec2::sub_inplace(Vec2 rhs)
{
    this->x -= rhs.x;
    this->y -= rhs.y;
}

void Vec2::mul_inplace(Vec2 rhs)
{
    this->x *= rhs.x;
    this->y *= rhs.y;
}

void Vec2::div_inplace(Vec2 rhs)
{
    this->x /= rhs.x;
    this->y /= rhs.y;
}

void Vec2::negate()
{
    this->x *= -1;
    this->y *= -1;
}

void Vec2::normalize()
{
    f32 mag_inv = 1 / this->magninute();
    this->x *= mag_inv;
    this->y *= mag_inv;
}

f32 Vec2::magninute()
{
    return std::sqrt(this->magninute_square());
}

f32 Vec2::magninute_square()
{
    f32 x = this->x;
    f32 y = this->y;
    return x*x + y*y;
}

Vec2 vec2_direction(Vec2 a, Vec2 b)
{
    return { b.x - a.x, b.y - a.y };
}

f32 vec2_dist(Vec2 a, Vec2 b)
{
    return vec2_direction(a, b).magninute();
}

Vec2 operator+(Vec2 a, Vec2 b) { return vec2_add(a, b); }
Vec2 operator-(Vec2 a, Vec2 b) { return vec2_sub(a, b); }
Vec2 operator*(Vec2 a, Vec2 b) { return vec2_mul(a, b); }
Vec2 operator/(Vec2 a, Vec2 b) { return vec2_div(a, b); }

// Vec3.

Vec3 vec3_add(Vec3 lhs, Vec3 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    Vec3 res;
    __m128 a = _mm_load_ps(lhs.arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 add_res = _mm_add_ps(a, b);
    _mm_store_ps(res.arr, add_res);
    return res;
#else
    return { this->x + rhs.x, this->y + rhs.y, this->z + rhs.z };
#endif
}

Vec3 vec3_sub(Vec3 lhs, Vec3 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    Vec3 res;
    __m128 a = _mm_load_ps(lhs.arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 sub_res = _mm_sub_ps(a, b);
    _mm_store_ps(res.arr, sub_res);
    return res;
#else
    return { this->x - rhs.x, this->y - rhs.y, this->z - rhs.z };
#endif
}

Vec3 vec3_mul(Vec3 lhs, Vec3 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    Vec3 res;
    __m128 a = _mm_load_ps(lhs.arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 mul_res = _mm_mul_ps(a, b);
    _mm_store_ps(res.arr, mul_res);
    return res;
#else
    return { this->x * rhs.x, this->y * rhs.y, this->z * rhs.z };
#endif
}

Vec3 vec3_div(Vec3 lhs, Vec3 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    Vec3 res;
    __m128 a = _mm_load_ps(lhs.arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 div_res = _mm_div_ps(a, b);
    _mm_store_ps(res.arr, div_res);
    return res;
#else
    return { this->x / rhs.x, this->y / rhs.y, this->z / rhs.z };
#endif
}

void Vec3::add_inplace(Vec3 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(this->arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 add_res = _mm_add_ps(a, b);
    _mm_store_ps(this->arr, add_res);
#else
    this->x += rhs.x;
    this->y += rhs.y;
    this->z += rhs.z;
#endif
}

void Vec3::sub_inplace(Vec3 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(this->arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 sub_res = _mm_sub_ps(a, b);
    _mm_store_ps(this->arr, sub_res);
#else
    this->x -= rhs.x;
    this->y -= rhs.y;
    this->z -= rhs.z;
#endif
}

void Vec3::mul_inplace(Vec3 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(this->arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 mul_res = _mm_mul_ps(a, b);
    _mm_store_ps(this->arr, mul_res);
#else
    this->x *= rhs.x;
    this->y *= rhs.y;
    this->z *= rhs.z;
#endif
}

void Vec3::div_inplace(Vec3 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(this->arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 div_res = _mm_div_ps(a, b);
    _mm_store_ps(this->arr, div_res);
#else
    this->x /= rhs.x;
    this->y /= rhs.y;
    this->z /= rhs.z;
#endif
}

void Vec3::negate()
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(this->arr);
    __m128 b = _mm_set1_ps(-1.0f);
    __m128 mul_res = _mm_mul_ps(a, b);
    _mm_store_ps(this->arr, mul_res);
#else
    this->x *= -1;
    this->y *= -1;
    this->z *= -1;
#endif
}

void Vec3::normalize()
{
    f32 mag_inv = 1 / this->magninute();
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(this->arr);
    __m128 b = _mm_set1_ps(mag_inv);
    __m128 mul_res = _mm_mul_ps(a, b);
    _mm_store_ps(this->arr, mul_res);
#else
    this->x *= mag_inv;
    this->y *= mag_inv;
    this->z *= mag_inv;
#endif
}

f32 vec3_dot(Vec3 lhs, Vec3 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(lhs.arr);
    __m128 b = _mm_load_ps(rhs.arr);
    return dot_f128(a, b);
#else
    return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z;
#endif
}

Vec3 vec3_cross(Vec3 lhs, Vec3 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    Vec3 res;
    __m128 a = _mm_load_ps(lhs.arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 cross_res = cross_f128(a, b);
    _mm_store_ps(res.arr, cross_res);
    return res;
#else
    return {
        this->y * rhs.z - this->z * rhs.y,
        this->z * rhs.x - this->x * rhs.z,
        this->x * rhs.y - this->y * rhs.x,
    };
#endif
}

f32 Vec3::magninute()
{
    return std::sqrt(this->magninute_square());
}

f32 Vec3::magninute_square()
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(this->arr);
    __m128 b = _mm_load_ps(this->arr);
    return dot_f128(a, b);
#else
    f32 x = this->x;
    f32 y = this->y;
    f32 z = this->z;
    return x*x + y*y + z*z;
#endif
}

Vec3 vec3_direction(Vec3 lhs, Vec3 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    Vec3 res;
    __m128 a_ = _mm_load_ps(lhs.arr);
    __m128 b_ = _mm_load_ps(rhs.arr);
    __m128 sub_res = _mm_sub_ps(b_, a_);
    _mm_store_ps(res.arr, sub_res);
    return res;
#else
    return { b.x - a.x, b.y - a.y, b.z - a.z };
#endif
}

f32 vec3_dist(Vec3 a, Vec3 b)
{
    return vec3_direction(a, b).magninute();
}

Vec3 operator+(Vec3 a, Vec3 b) { return vec3_add(a, b); }
Vec3 operator-(Vec3 a, Vec3 b) { return vec3_sub(a, b); }
Vec3 operator*(Vec3 a, Vec3 b) { return vec3_mul(a, b); }
Vec3 operator/(Vec3 a, Vec3 b) { return vec3_div(a, b); }

// Vec4.

Vec4 vec4_add(Vec4 lhs, Vec4 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    Vec4 res;
    __m128 a = _mm_load_ps(lhs.arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 add_res = _mm_add_ps(a, b);
    _mm_store_ps(res.arr, add_res);
    return res;
#else
    return { this->x + rhs.x, this->y + rhs.y, this->z + rhs.z };
#endif
}

Vec4 vec4_sub(Vec4 lhs, Vec4 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(lhs.arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 add_res = _mm_sub_ps(a, b);
    Vec4 res;
    _mm_store_ps(res.arr, add_res);
    return res;
#else
    return { this->x - rhs.x, this->y - rhs.y, this->z - rhs.z };
#endif
}

Vec4 vec4_mul(Vec4 lhs, Vec4 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(lhs.arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 add_res = _mm_mul_ps(a, b);
    Vec4 res;
    _mm_store_ps(res.arr, add_res);
    return res;
#else
    return { this->x * rhs.x, this->y * rhs.y, this->z * rhs.z };
#endif
}

Vec4 vec4_div(Vec4 lhs, Vec4 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(lhs.arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 add_res = _mm_div_ps(a, b);
    Vec4 res;
    _mm_store_ps(res.arr, add_res);
    return res;
#else
    return { this->x / rhs.x, this->y / rhs.y, this->z / rhs.z };
#endif
}

f32 vec4_dot(Vec4 lhs, Vec4 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(lhs.arr);
    __m128 b = _mm_load_ps(rhs.arr);
    return dot_f128(a, b);
#else
    return this->x * rhs.x + this->y * rhs.y + this->z * rhs.z;
#endif
}

Vec4 vec4_cross(Vec4 lhs, Vec4 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    Vec4 res;
    __m128 a = _mm_load_ps(lhs.arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 cross_res = cross_f128(a, b);
    _mm_store_ps(res.arr, cross_res);
    return res;
#else
    return {
        this->y * rhs.z - this->z * rhs.y,
        this->z * rhs.x - this->x * rhs.z,
        this->x * rhs.y - this->y * rhs.x,
    };
#endif
}

void Vec4::add_inplace(Vec4 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(this->arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 add_res = _mm_add_ps(a, b);
    _mm_store_ps(this->arr, add_res);
#else
    this->x += rhs.x;
    this->y += rhs.y;
    this->z += rhs.z;
#endif
}

void Vec4::sub_inplace(Vec4 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(this->arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 sub_res = _mm_sub_ps(a, b);
    _mm_store_ps(this->arr, sub_res);
#else
    this->x -= rhs.x;
    this->y -= rhs.y;
    this->z -= rhs.z;
#endif
}

void Vec4::mul_inplace(Vec4 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(this->arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 mul_res = _mm_mul_ps(a, b);
    _mm_store_ps(this->arr, mul_res);
#else
    this->x *= rhs.x;
    this->y *= rhs.y;
    this->z *= rhs.z;
#endif
}

void Vec4::div_inplace(Vec4 rhs)
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(this->arr);
    __m128 b = _mm_load_ps(rhs.arr);
    __m128 div_res = _mm_div_ps(a, b);
    _mm_store_ps(this->arr, div_res);
#else
    this->x /= rhs.x;
    this->y /= rhs.y;
    this->z /= rhs.z;
#endif
}

void Vec4::negate()
{
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(this->arr);
    __m128 b = _mm_set1_ps(-1.0f);
    __m128 res = _mm_mul_ps(a, b);
    _mm_store_ps(this->arr, res);
#else
    this->x *= -1;
    this->y *= -1;
    this->z *= -1;
#endif
}

void Vec4::normalize()
{
    f32 mag_inv = 1 / this->magninute();
#ifdef RG_FEATURE_SIMD_128
    __m128 a = _mm_load_ps(this->arr);
    __m128 b = _mm_set1_ps(mag_inv);
    __m128 res = _mm_mul_ps(a, b);
    _mm_store_ps(this->arr, res);
#else
    this->x *= mag_inv;
    this->y *= mag_inv;
    this->z *= mag_inv;
#endif
}

f32 Vec4::magninute()
{
    return std::sqrt(this->magninute_square());
}

f32 Vec4::magninute_square()
{
#ifdef RG_FEATURE_SIMD_128
    alignas(16) f32 load_buff[4];
    rg::mem_copy(load_buff, this->arr, sizeof(f32) * 4);
    __m128 a = _mm_load_ps(load_buff);
    __m128 b = _mm_load_ps(load_buff);
    return dot_f128(a, b);
#else
    f32 x = this->x;
    f32 y = this->y;
    f32 z = this->z;
    return x*x + y*y + z*z;
#endif
}

Vec4 vec4_direction(Vec4 a, Vec4 b)
{
    Vec4 res;
#ifdef RG_FEATURE_SIMD_128
    __m128 a_ = _mm_load_ps(a.arr);
    __m128 b_ = _mm_load_ps(b.arr);
    __m128 sub_res = _mm_sub_ps(b_, a_);
    _mm_store_ps(res.arr, sub_res);
    return res;
#else
    return { b.x - a.x, b.y - a.y, b.z - a.z };
#endif
}

f32 vec4_dist(Vec4 a, Vec4 b)
{
    return vec4_direction(a, b).magninute();
}

Vec4 operator+(Vec4 a, Vec4 b) { return vec4_add(a, b); }
Vec4 operator-(Vec4 a, Vec4 b) { return vec4_sub(a, b); }
Vec4 operator*(Vec4 a, Vec4 b) { return vec4_mul(a, b); }
Vec4 operator/(Vec4 a, Vec4 b) { return vec4_div(a, b); }

// Mat4.

Mat4 Mat4::identity()
{
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
}

Mat4 Mat4::init_translate(Vec3 v)
{
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        v.x, v.y, v.z, 1
    };
}

Mat4 Mat4::init_scale(Vec3 v)
{
    return {
        v.x, 0, 0, 0,
        0, v.y, 0, 0,
        0, 0, v.z, 0,
        0, 0, 0, 1
    };
}

void Mat4::translate_inplace(Vec3 v)
{
#ifdef RG_FEATURE_SIMD_128
     f32* m = this->data;

    __m128 r0 = _mm_load_ps(&m[0]);
    __m128 r1 = _mm_load_ps(&m[4]);
    __m128 r2 = _mm_load_ps(&m[8]);
    __m128 r3 = _mm_load_ps(&m[12]);

    __m128 vx = _mm_set1_ps(v.x);
    __m128 vy = _mm_set1_ps(v.y);
    __m128 vz = _mm_set1_ps(v.z);

    __m128 r3_new = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(r0, vx), _mm_mul_ps(r1, vy)),
        _mm_add_ps(_mm_mul_ps(r2, vz), r3)
    );

    _mm_storeu_ps(&m[12], r3_new);
#else
    f32* m = this->data;
	m[12] = m[0] * v.x + m[4] * v.y + m[8]  * v.z + m[12];
	m[13] = m[1] * v.x + m[5] * v.y + m[9]  * v.z + m[13];
	m[14] = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
	m[15] = m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15];
#endif
}

void Mat4::scale_inplace(Vec3 v)
{
#ifdef RG_FEATURE_SIMD_128
    f32* m = this->data;

    __m128 vx = _mm_set1_ps(v.x);
    __m128 vy = _mm_set1_ps(v.y);
    __m128 vz = _mm_set1_ps(v.z);

    __m128 r0 = _mm_load_ps(&m[0]);
    __m128 r1 = _mm_load_ps(&m[4]);
    __m128 r2 = _mm_load_ps(&m[8]);

    r0 = _mm_mul_ps(r0, vx);
    r1 = _mm_mul_ps(r1, vy);
    r2 = _mm_mul_ps(r2, vz);

    _mm_storeu_ps(&m[0], r0);
    _mm_storeu_ps(&m[4], r1);
    _mm_storeu_ps(&m[8], r2);
#else
    Mat4 rhs = { v.x, v.x, v.x, v.x, v.y, v.y, v.y, v.y, v.z, v.z, v.z, v.z, 1, 1, 1, 1 };
    this->mul_inplace(rhs);
#endif
}

Mat4 Mat4::translate(Vec3 v) const
{
    Mat4 rhs = *this;
    rhs.translate_inplace(v);
    return rhs;
}

Mat4 Mat4::scale(Vec3 v) const
{
    Mat4 res = *this;
    res.scale_inplace(v);
    return res;
}

void Mat4::mul_inplace(const Mat4& rhs)
{
#ifdef RG_FEATURE_SIMD_128
    f32* src = this->data;
    const f32* b = rhs.data;

    __m128 b0 = _mm_load_ps(&b[0]);
    __m128 b1 = _mm_load_ps(&b[4]);
    __m128 b2 = _mm_load_ps(&b[8]);
    __m128 b3 = _mm_load_ps(&b[12]);

    for (s32 i = 0; i < 16; i += 4) 
    {
        __m128 a0 = _mm_set1_ps(src[i + 0]);
        __m128 a1 = _mm_set1_ps(src[i + 1]); 
        __m128 a2 = _mm_set1_ps(src[i + 2]); 
        __m128 a3 = _mm_set1_ps(src[i + 3]); 

        __m128 r = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(a0, b0), _mm_mul_ps(a1, b1)),
            _mm_add_ps(_mm_mul_ps(a2, b2), _mm_mul_ps(a3, b3))
        );

        _mm_storeu_ps(&src[i], r);
    }
#else
    Array<f32, 16>& src = *this;
    // Copy.
    const Array<f32, 16> a = *this;
    const Array<f32, 16>& b = rhs;

    src[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
    src[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
    src[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
    src[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

    src[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
    src[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
    src[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
    src[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

    src[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
    src[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
    src[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
    src[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

    src[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
    src[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
    src[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
    src[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
#endif
}

Mat4 mat4_mul(const Mat4& lhs, const Mat4& rhs)
{
    Mat4 res = lhs;
    res.mul_inplace(rhs);
    return res;
}

void Mat4::transpose_inplace()
{
    f32* m = this->data;
#ifdef RG_FEATURE_SIMD_128
    __m128 r0 = _mm_load_ps(&m[0]);
    __m128 r1 = _mm_load_ps(&m[4]);
    __m128 r2 = _mm_load_ps(&m[8]);
    __m128 r3 = _mm_load_ps(&m[12]);

    __m128 t0 = _mm_unpacklo_ps(r0, r1); 
    __m128 t1 = _mm_unpackhi_ps(r0, r1); 
    __m128 t2 = _mm_unpacklo_ps(r2, r3); 
    __m128 t3 = _mm_unpackhi_ps(r2, r3); 

    r0 = _mm_movelh_ps(t0, t2); 
    r1 = _mm_movehl_ps(t2, t0); 
    r2 = _mm_movelh_ps(t1, t3); 
    r3 = _mm_movehl_ps(t3, t1); 

    _mm_storeu_ps(&m[0],  r0);
    _mm_storeu_ps(&m[4],  r1);
    _mm_storeu_ps(&m[8],  r2);
    _mm_storeu_ps(&m[12], r3);
#else
    rg::swap(&m[1],  &m[4]);
    rg::swap(&m[2],  &m[8]);
    rg::swap(&m[3],  &m[12]);
    rg::swap(&m[6],  &m[9]);
    rg::swap(&m[7],  &m[13]);
    rg::swap(&m[11], &m[14]);
#endif
}

Mat4 Mat4::transpose() const
{
    Mat4 m = *this;
    m.transpose_inplace();
    return m;
}

// Inverts matrix which only was affected by translation or scale.
void Mat4::invert_affine()
{
    f32* m = this->data;
#ifdef RG_FEATURE_SIMD_128
    __m128 r0 = _mm_load_ps(&m[0]);
    __m128 r1 = _mm_load_ps(&m[4]);
    __m128 r2 = _mm_load_ps(&m[8]);

    __m128 t0 = _mm_unpacklo_ps(r0, r1);
    __m128 t1 = _mm_unpackhi_ps(r0, r1);

    __m128 r0_new = _mm_movelh_ps(t0, r2);
    __m128 r1_new = _mm_movehl_ps(r2, t0);
    __m128 r2_new = _mm_shuffle_ps(t1, r2, _MM_SHUFFLE(3, 2, 1, 0));

    __m128 mask_xyz = _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
    r0_new = _mm_and_ps(r0_new, mask_xyz);
    r1_new = _mm_and_ps(r1_new, mask_xyz);
    r2_new = _mm_and_ps(r2_new, mask_xyz);

    __m128 r3_old = _mm_load_ps(&m[12]); 
    
    __m128 tx = _mm_shuffle_ps(r3_old, r3_old, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 ty = _mm_shuffle_ps(r3_old, r3_old, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 tz = _mm_shuffle_ps(r3_old, r3_old, _MM_SHUFFLE(2, 2, 2, 2));

    __m128 r3_new = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(tx, r0_new), _mm_mul_ps(ty, r1_new)),
        _mm_mul_ps(tz, r2_new)
    );
    r3_new = _mm_sub_ps(_mm_setzero_ps(), r3_new); // Negate 

    alignas(16) float final_row[4];
    _mm_store_ps(final_row, r3_new);
    final_row[3] = 1.0f;

    _mm_storeu_ps(&m[0], r0_new);
    _mm_storeu_ps(&m[4], r1_new);
    _mm_storeu_ps(&m[8], r2_new);
    _mm_storeu_ps(&m[12], _mm_load_ps(final_row));
#else
    rg::swap(&m[1], &m[4]);
    rg::swap(&m[2], &m[8]);
    rg::swap(&m[6], &m[9]);

    f32 tx = m[12];
    f32 ty = m[13];
    f32 tz = m[14];

    m[12] = -(m[0] * tx + m[4] * ty + m[8]  * tz);
    m[13] = -(m[1] * tx + m[5] * ty + m[9]  * tz);
    m[14] = -(m[2] * tx + m[6] * ty + m[10] * tz);
    m[15] = 1.0f;
#endif
}

// Uses cramer-rule, very heavy, but rarely needed.
void Mat4::invert_general()
{
    f32* m = this->data;
#ifdef RG_FEATURE_SIMD_128
    // --- 1. Load Rows ---
    __m128 src0 = _mm_load_ps(&m[0]);
    __m128 src1 = _mm_load_ps(&m[4]);
    __m128 src2 = _mm_load_ps(&m[8]);
    __m128 src3 = _mm_load_ps(&m[12]);

    // --- 2. Transpose Input ---
    // Intel's algorithm requires the input to be transposed first so it can
    // compute the cofactors efficiently using parallel horizontal shuffling.
    __m128 t0 = _mm_unpacklo_ps(src0, src1);
    __m128 t1 = _mm_unpackhi_ps(src0, src1);
    __m128 t2 = _mm_unpacklo_ps(src2, src3);
    __m128 t3 = _mm_unpackhi_ps(src2, src3);
    
    __m128 row0 = _mm_movelh_ps(t0, t2);
    __m128 row1 = _mm_movehl_ps(t2, t0);
    __m128 row2 = _mm_movelh_ps(t1, t3);
    __m128 row3 = _mm_movehl_ps(t3, t1);

    // --- 3. Compute 2x2 Sub-Determinants (Pairs) ---
    __m128 tmp1, fac0, fac1, fac2, fac3, fac4, fac5;

    tmp1 = _mm_shuffle_ps(row2, row2, _MM_SHUFFLE(1, 0, 3, 2));
    fac0 = _mm_mul_ps(tmp1, _mm_shuffle_ps(row3, row3, _MM_SHUFFLE(2, 3, 0, 1)));
    fac1 = _mm_mul_ps(tmp1, _mm_shuffle_ps(row3, row3, _MM_SHUFFLE(3, 2, 1, 0)));
    tmp1 = _mm_shuffle_ps(row2, row2, _MM_SHUFFLE(0, 2, 1, 3));
    fac2 = _mm_mul_ps(tmp1, _mm_shuffle_ps(row3, row3, _MM_SHUFFLE(1, 0, 3, 2)));
    fac3 = _mm_mul_ps(tmp1, _mm_shuffle_ps(row3, row3, _MM_SHUFFLE(2, 3, 0, 1)));
    tmp1 = _mm_shuffle_ps(row2, row2, _MM_SHUFFLE(2, 1, 0, 3));
    fac4 = _mm_mul_ps(tmp1, _mm_shuffle_ps(row3, row3, _MM_SHUFFLE(3, 2, 1, 0)));
    fac5 = _mm_mul_ps(tmp1, _mm_shuffle_ps(row3, row3, _MM_SHUFFLE(0, 1, 2, 3)));

    // --- 4. Compute Adjugate Matrix Rows ---
    __m128 dst0, dst1, dst2, dst3;

    tmp1 = _mm_shuffle_ps(row1, row1, _MM_SHUFFLE(1, 0, 3, 2));
    dst0 = _mm_mul_ps(tmp1, _mm_shuffle_ps(fac0, fac0, _MM_SHUFFLE(2, 3, 0, 1)));
    dst1 = _mm_mul_ps(tmp1, _mm_shuffle_ps(fac1, fac1, _MM_SHUFFLE(3, 2, 1, 0)));
    dst2 = _mm_mul_ps(tmp1, _mm_shuffle_ps(fac2, fac2, _MM_SHUFFLE(1, 0, 3, 2)));
    dst3 = _mm_mul_ps(tmp1, _mm_shuffle_ps(fac3, fac3, _MM_SHUFFLE(2, 3, 0, 1)));

    tmp1 = _mm_shuffle_ps(row1, row1, _MM_SHUFFLE(2, 3, 0, 1));
    dst0 = _mm_sub_ps(dst0, _mm_mul_ps(tmp1, _mm_shuffle_ps(fac0, fac0, _MM_SHUFFLE(1, 0, 3, 2))));
    dst1 = _mm_sub_ps(dst1, _mm_mul_ps(tmp1, _mm_shuffle_ps(fac1, fac1, _MM_SHUFFLE(0, 1, 2, 3))));
    dst2 = _mm_sub_ps(dst2, _mm_mul_ps(tmp1, _mm_shuffle_ps(fac4, fac4, _MM_SHUFFLE(3, 2, 1, 0))));
    dst3 = _mm_sub_ps(dst3, _mm_mul_ps(tmp1, _mm_shuffle_ps(fac5, fac5, _MM_SHUFFLE(2, 3, 0, 1))));

    tmp1 = _mm_shuffle_ps(row1, row1, _MM_SHUFFLE(3, 2, 1, 0));
    dst0 = _mm_add_ps(dst0, _mm_mul_ps(tmp1, _mm_shuffle_ps(fac1, fac1, _MM_SHUFFLE(1, 0, 3, 2))));
    dst1 = _mm_sub_ps(_mm_mul_ps(tmp1, _mm_shuffle_ps(fac0, fac0, _MM_SHUFFLE(0, 1, 2, 3))), dst1);
    dst2 = _mm_add_ps(dst2, _mm_mul_ps(tmp1, _mm_shuffle_ps(fac5, fac5, _MM_SHUFFLE(1, 0, 3, 2))));
    dst3 = _mm_sub_ps(_mm_mul_ps(tmp1, _mm_shuffle_ps(fac4, fac4, _MM_SHUFFLE(0, 1, 2, 3))), dst3);

    // Apply alternating cofactor signs to destination components
    __m128 sign_mask = _mm_setr_ps(1.0f, -1.0f, 1.0f, -1.0f);
    __m128 sign_mask_inv = _mm_setr_ps(-1.0f, 1.0f, -1.0f, 1.0f);
    
    dst0 = _mm_mul_ps(dst0, sign_mask);
    dst1 = _mm_mul_ps(dst1, sign_mask_inv);
    dst2 = _mm_mul_ps(dst2, sign_mask);
    dst3 = _mm_mul_ps(dst3, sign_mask_inv);

    // --- 5. Calculate Matrix Determinant ---
    __m128 det = _mm_mul_ps(row0, dst0);
    det = _mm_add_ps(det, _mm_shuffle_ps(det, det, 0x4E));
    det = _mm_add_ps(det, _mm_shuffle_ps(det, det, 0xB1));
    
    __m128 zero = _mm_setzero_ps();
    if (_mm_comieq_ss(det, zero)) {
        return; // Singular matrix, inverse does not exist
    }
    
    // --- 6. Apply Reciprocal Determinant and Store ---
    __m128 rcp = _mm_div_ps(_mm_set1_ps(1.0f), det);
    
    _mm_storeu_ps(&m[0],  _mm_mul_ps(dst0, rcp));
    _mm_storeu_ps(&m[4],  _mm_mul_ps(dst1, rcp));
    _mm_storeu_ps(&m[8],  _mm_mul_ps(dst2, rcp));
    _mm_storeu_ps(&m[12], _mm_mul_ps(dst3, rcp));
#else
    // --- Scalar Cramer's Rule for Row-Major Matrix ---
    // Compute 2x2 sub-determinants of the bottom two rows
    f32 s0 = m[10] * m[15] - m[11] * m[14];
    f32 s1 = m[9]  * m[15] - m[11] * m[13];
    f32 s2 = m[9]  * m[14] - m[10] * m[13];
    f32 s3 = m[8]  * m[15] - m[11] * m[12];
    f32 s4 = m[8]  * m[14] - m[10] * m[12];
    f32 s5 = m[8]  * m[13] - m[9]  * m[12];

    // Compute 2x2 sub-determinants of the top two rows
    f32 c5 = m[2] * m[7] - m[3] * m[6];
    f32 c4 = m[1] * m[7] - m[3] * m[5];
    f32 c3 = m[1] * m[6] - m[2] * m[5];
    f32 c2 = m[0] * m[7] - m[3] * m[4];
    f32 c1 = m[0] * m[6] - m[2] * m[4];
    f32 c0 = m[0] * m[5] - m[1] * m[4];

    // Calculate the overall matrix determinant
    f32 det = m[0] * ( m[5] * s0 - m[6] * s1 + m[7] * s2)
            - m[1] * ( m[4] * s0 - m[6] * s3 + m[7] * s4)
            + m[2] * ( m[4] * s1 - m[5] * s3 + m[7] * s5)
            - m[3] * ( m[4] * s2 - m[5] * s4 + m[6] * s5);

    if (det == 0.0f) {
        return; // Singular matrix
    }

    f32 inv_det = 1.0f / det;

    // Cache components to prevent inline destination corruption
    f32 m0 = m[0],  m1 = m[1],  m2 = m[2],  m3 = m[3];
    f32 m4 = m[4],  m5 = m[5],  m6 = m[6],  m7 = m[7];
    f32 m8 = m[8],  m9 = m[9],  m10= m[10], m11= m[11];
    f32 m12= m[12], m13= m[13], m14= m[14], m15= m[15];

    // Row 0
    m[0]  = ( m5 * s0 - m6 * s1 + m7 * s2) * inv_det;
    m[1]  = (-m1 * s0 + m2 * s1 - m3 * s2) * inv_det;
    m[2]  = ( m13* c5 - m14* c4 + m15* c3) * inv_det;
    m[3]  = (-m9 * c5 + m10* c4 - m11* c3) * inv_det;

    // Row 1
    m[4]  = (-m4 * s0 + m6 * s3 - m7 * s4) * inv_det;
    m[5]  = ( m0 * s0 - m2 * s3 + m3 * s4) * inv_det;
    m[6]  = (-m12* c5 + m14* c2 - m15* c1) * inv_det;
    m[7]  = ( m8 * c5 - m10* c2 + m11* c1) * inv_det;

    // Row 2
    m[8]  = ( m4 * s1 - m5 * s3 + m7 * s5) * inv_det;
    m[9]  = (-m0 * s1 + m1 * s3 - m3 * s5) * inv_det;
    m[10] = ( m12* c4 - m13* c2 + m15* c0) * inv_det;
    m[11] = (-m8 * c4 + m9 * c2 - m11* c0) * inv_det;

    // Row 3
    m[12] = (-m4 * s2 + m5 * s4 - m6 * s5) * inv_det;
    m[13] = ( m0 * s2 - m1 * s4 + m2 * s5) * inv_det;
    m[14] = (-m12* c3 + m13* c1 - m14* c0) * inv_det;
    m[15] = ( m8 * c3 - m9 * c1 + m10* c0) * inv_det;
#endif
}

// Quaternion.

// --- 1. Conjugate ---
// Inverts the rotation axis (negates X, Y, Z while keeping W positive)
Quat Quat::conjugate() const 
{
#ifdef RG_FEATURE_SIMD_128
    Quat result;
    __m128 q = _mm_load_ps(this->arr);
    // Flip signs of x, y, z using a sign mask
    __m128 mask = _mm_setr_ps(-1.0f, -1.0f, -1.0f, 1.0f);
    _mm_store_ps(result.arr, _mm_mul_ps(q, mask));
    return result;
#else
    return { -x, -y, -z, w };
#endif
}

// --- 2. Multiplication (operator*) ---
// Combines two rotations. order matters: lhs * rhs means apply lhs THEN rhs in Row-Major.
Quat operator*(Quat lhs, Quat rhs)
{
    Quat result;
#ifdef RG_FEATURE_SIMD_128
    __m128 l = _mm_load_ps(lhs.arr);
    __m128 r = _mm_load_ps(rhs.arr);

    // SIMD horizontal expansion of quaternion multiplication formula
    __m128 l_wwww = _mm_shuffle_ps(l, l, _MM_SHUFFLE(3, 3, 3, 3));
    __m128 l_xxxx = _mm_shuffle_ps(l, l, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 l_yyyy = _mm_shuffle_ps(l, l, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 l_zzzz = _mm_shuffle_ps(l, l, _MM_SHUFFLE(2, 2, 2, 2));

    __m128 r_wzyx = _mm_shuffle_ps(r, r, _MM_SHUFFLE(3, 2, 1, 0));
    __m128 r_zwxy = _mm_shuffle_ps(r, r, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 r_yxwz = _mm_shuffle_ps(r, r, _MM_SHUFFLE(1, 0, 3, 2));

    __m128 m0 = _mm_mul_ps(l_wwww, r);
    __m128 m1 = _mm_mul_ps(l_xxxx, r_wzyx);
    __m128 m2 = _mm_mul_ps(l_yyyy, r_zwxy);
    __m128 m3 = _mm_mul_ps(l_zzzz, r_yxwz);

    // Sign patterns for multiplication components
    __m128 s1 = _mm_setr_ps( 1.0f, -1.0f,  1.0f, -1.0f);
    __m128 s2 = _mm_setr_ps( 1.0f,  1.0f, -1.0f, -1.0f);
    __m128 s3 = _mm_setr_ps(-1.0f,  1.0f,  1.0f, -1.0f);

    __m128 q_res = _mm_add_ps(m0, _mm_mul_ps(m1, s1));
    q_res = _mm_add_ps(q_res, _mm_mul_ps(m2, s2));
    q_res = _mm_add_ps(q_res, _mm_mul_ps(m3, s3));

    _mm_store_ps(result.arr, q_res);
#else
    f32 x1 = lhs.x, y1 = lhs.y, z1 = lhs.z, w1 = lhs.w;
    f32 x2 = rhs.x, y2 = rhs.y, z2 = rhs.z, w2 = rhs.w;

    result.x = w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2;
    result.y = w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2;
    result.z = w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2;
    result.w = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2;
#endif
    return result;
}

// --- 3. Convert to Row-Major Mat4 ---
Mat4 Quat::to_matrix() const 
{
    Mat4 mat;
    f32* m = mat.data;

    f32 x = arr[0], y = arr[1], z = arr[2], w = arr[3];
    
    f32 xx = x * x, xy = x * y, xz = x * z, xw = x * w;
    f32 yy = y * y, yz = y * z, yw = y * w;
    f32 zz = z * z, zw_val = z * w;

    // Row 0
    m[0] = 1.0f - 2.0f * (yy + zz);
    m[1] = 2.0f * (xy + zw_val);
    m[2] = 2.0f * (xz - yw);
    m[3] = 0.0f;

    // Row 1
    m[4] = 2.0f * (xy - zw_val);
    m[5] = 1.0f - 2.0f * (xx + zz);
    m[6] = 2.0f * (yz + xw);
    m[7] = 0.0f;

    // Row 2
    m[8]  = 2.0f * (xz + yw);
    m[9]  = 2.0f * (yz - xw);
    m[10] = 1.0f - 2.0f * (xx + yy);
    m[11] = 0.0f;

    // Row 3 (Translation block remains completely baseline identity)
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;

    return mat;
}

// --- 4. Construct from Euler Angles (YXZ/Yaw-Pitch-Roll order) ---
Quat Quat::create_from_euler(f32 pitch, f32 yaw, f32 roll) 
{
    f32 heading = yaw;
    f32 attitude = pitch;
    f32 bank = roll;

    f32 c1 = std::cos(heading / 2.0f);
    f32 s1 = std::sin(heading / 2.0f);
    f32 c2 = std::cos(attitude / 2.0f);
    f32 s2 = std::sin(attitude / 2.0f);
    f32 c3 = std::cos(bank / 2.0f);
    f32 s3 = std::sin(bank / 2.0f);

    // Combines components into a single step without matrix overhead
    return {
        s1 * c2 * c3 + c1 * s2 * s3,
        c1 * s2 * c3 - s1 * c2 * s3,
        c1 * c2 * s3 - s1 * s2 * c3,
        c1 * c2 * c3 + s1 * s2 * s3
    };
}

// Underlying operations.

// Simd128.

#ifdef RG_FEATURE_SIMD_128

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
    f128 prod = _mm_mul_ps(a, b); 
    f128 shuf1 = _mm_shuffle_ps(prod, prod, _MM_SHUFFLE(2, 3, 0, 1));
    f128 sum1  = _mm_add_ps(prod, shuf1);
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

#endif // Simd 128

// Simd 256.

#ifdef RG_FEATURE_SIMD_256

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

#endif // Simd256

} // rg
