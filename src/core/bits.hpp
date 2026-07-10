#ifndef _RG_BITS_HPP_
#define _RG_BITS_HPP_

#include "core/basic.hpp"

namespace rg
{

// Common masks.
enum BitMask64 : u64
{
    BIT_MASK_64_U8_LOW  = 0x00000000000000FF,
    BIT_MASK_64_U8_HIGH  = 0xFF00000000000000,
    BIT_MASK_64_U16_LOW = 0x000000000000FFFF,
    BIT_MASK_64_U16_HIGH = 0xFFFF000000000000,
    BIT_MASK_64_U32_LOW = 0x00000000FFFFFFFF,
    BIT_MASK_64_U32_HIGH = 0xFFFFFFFF00000000,
    BIT_MASK_64_U56_LOW = 0x00FFFFFFFFFFFFFF,
    BIT_MASK_64_U56_HIGH = 0xFFFFFFFFFFFFFF00,
};

enum struct BitMask32 : u32
{
    BIT_MAKS_32_U8_LOW  = 0x000000FF,
    BIT_MAKS_32_U8_HIGH  = 0xFF000000,
    BIT_MAKS_32_U16_LOW = 0x0000FFFF,
    BIT_MAKS_32_U16_HIGH = 0xFFFF0000,
    BIT_MAKS_32_U24_LOW = 0x00FFFFFF,
    BIT_MAKS_32_U24_HIGH = 0xFFFFFF00,
};

template<typename Type = u64, typename Mask = BitMask64>
struct Bits
{
    static_assert(sizeof(Type) <= 8, "Mustn't exceed 64 bit for this type");
private:
    Type storage;
public:
    Type get(Mask mask) { return storage & mask; }
    void set(Type value, Mask mask)
    {
        storage &= ~mask; 
        storage |= mask & value;
    }
};

} // rg

#endif // _RG_BITS_HPP_
