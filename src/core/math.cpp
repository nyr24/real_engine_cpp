#include "math.hpp"

namespace rg
{

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
    // b contains rows 0, 1, 2, 3 as contiguous 128-bit blocks:
    // [ B3, B2, B1, B0 ] where each Bn is an __m128 (4 floats)
    // We can broadcast a single row of B across all 4 lanes of an __m512 register.

    // Broadcast Row 0 of B to all 4 quarters of a 512-bit register
    __m512 b_row0 = _mm512_shuffle_f32x4(b.repr, b.repr, _MM_SHUFFLE(0, 0, 0, 0));
    // Broadcast Row 1 of B
    __m512 b_row1 = _mm512_shuffle_f32x4(b.repr, b.repr, _MM_SHUFFLE(1, 1, 1, 1));
    // Broadcast Row 2 of B
    __m512 b_row2 = _mm512_shuffle_f32x4(b.repr, b.repr, _MM_SHUFFLE(2, 2, 2, 2));
    // Broadcast Row 3 of B
    __m512 b_row3 = _mm512_shuffle_f32x4(b.repr, b.repr, _MM_SHUFFLE(3, 3, 3, 3));

    // Now, we broadcast the corresponding column elements of A across 128-bit boundaries.
    // For element 0 of every row in A: indices (0, 4, 8, 12)
    __m512 a_col0 = _mm512_shuffle_ps(a.repr, a.repr, _MM_SHUFFLE(0, 0, 0, 0));
    
    // Step 1: Base multiplication (a_col0 * b_row0)
    __m512 res = _mm512_mul_ps(a_col0, b_row0);

    // For element 1 of every row in A: indices (1, 5, 9, 13)
    __m512 a_col1 = _mm512_shuffle_ps(a.repr, a.repr, _MM_SHUFFLE(1, 1, 1, 1));
    // Step 2: Fused Multiply-Add
    res = _mm512_fmadd_ps(a_col1, b_row1, res);

    // For element 2 of every row in A: indices (2, 6, 10, 14)
    __m512 a_col2 = _mm512_shuffle_ps(a.repr, a.repr, _MM_SHUFFLE(2, 2, 2, 2));
    // Step 3: Fused Multiply-Add
    res = _mm512_fmadd_ps(a_col2, b_row2, res);

    // For element 3 of every row in A: indices (3, 7, 11, 15)
    __m512 a_col3 = _mm512_shuffle_ps(a.repr, a.repr, _MM_SHUFFLE(3, 3, 3, 3));
    // Step 4: Final Fused Multiply-Add
    res = _mm512_fmadd_ps(a_col3, b_row3, res);

    return Mat4{ res };
}

// 8bit.

// void cmp_string(Slice<char> str)
// {
//  // 1. Исходные 16 символов (например, ищем символ 'A')
//     const char text[16] = {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!', '!', 'A', '!', '!'};
//     char target = 'A';

//     // 2. Загружаем 16 символов в 128-битный регистр
//     __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(text));

//     // 3. Заполняем второй регистр эталонным символом (размножаем 'A' во все 16 слотов)
//     __m128i match_pattern = _mm_set1_epi8(target);

//     // 4. Сравниваем их. Если байты равны, в слоте будет 0xFF, если нет — 0x00
//     __m128i cmp_result = _mm_cmpeq_epi8(chunk, match_pattern);

//     // 5. Собираем старшие биты каждого байта в одно 16-битное число int (маску)
//     int mask = _mm_movemask_epi8(cmp_result);

//     // 6. Проверяем результат
//     if (mask != 0) {
//         std::cout << "Найдено совпадение! Битовая маска: " << std::hex << mask << "\n";
        
//         // Дополнительно: как узнать индекс первого совпавшего символа
//         // Инструкция подсчета хвостовых нулей (требует c++20 <bit> или __builtin_ctz)
//         int first_index = __builtin_ctz(mask); 
//         std::cout << "Первый совпавший символ находится на индексе: " << std::dec << first_index << "\n";
//     } else {
//         std::cout << "Совпадений нет.\n";
//     }
// }


// // Функция для безопасной сборки регистра из остатка данных (длиной от 1 до 15 байт)
// __m128i load_partial_safe(const char* src, size_t remaining_bytes) {
//     alignas(16) char buffer[16] = {0}; // Гарантированно обнуленный буфер
//     std::memcpy(buffer, src, remaining_bytes); // Копируем только то, что осталось
//     return _mm_load_si128(reinterpret_cast<const __m128i*>(buffer));
// }

// // Пример использования
// void process_string(const char* str, size_t length, char target) {
//     size_t i = 0;
//     __m128i match_pattern = _mm_set1_epi8(target);

//     // 1. Основной цикл по 16 байт
//     for (; i + 16 <= length; i += 16) {
//         __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(str + i));
//         int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, match_pattern));
//         if (mask != 0) { /* Обработка совпадения на индексе i + ctz(mask) */ }
//     }

//     // 2. Безопасная обработка остатка (хвоста строки)
//     size_t remaining = length - i;
//     if (remaining > 0) {
//         __m128i chunk = load_partial_safe(str + i, remaining);
//         int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, match_pattern));
        
//         // ВАЖНО: Нам нужно сбросить в ноль биты маски, которые вышли за реальную длину строки
//         mask &= (1 << remaining) - 1; 
        
//         if (mask != 0) { /* Обработка совпадения в хвосте */ }
//     }
// }

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
