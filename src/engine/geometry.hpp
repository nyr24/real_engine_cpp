#ifndef _RG_GEOMETRY_HPP_
#define _RG_GEOMETRY_HPP_

#include "core/bits.hpp"
#include "core/math.hpp"

namespace rg
{

struct Vertex
{
    Vec3 pos;
    Vec3 normal;
    Vec2 tex_coord;
};

enum IndexStride : u8
{
	INDEX_STRIDE_CHAR = 1,
	INDEX_STRIDE_SHORT = 2,
	INDEX_STRIDE_INT = 4,
};

struct GeometryView
{
    static constexpr u32 MASK_IS_INDEXED = 0b1;
    static constexpr u32 MASK_IDX_STRIDE = 0b1110;
    static constexpr u32 MASK_COUNT = ~u32(0) & ~(MASK_IDX_STRIDE | MASK_IS_INDEXED);
    
    // Uses u32 as internal storage type to pack data better.
    // [count=31..5;idx_stride=4..1;is_indexed=0..0]
	BitInt<u32> bits;
    u32 indices_offset;
    u32 vertices_offset;
	// Either vertex_byte_size of index_byte_size, depending on 'is_indexed'
	u32 size_bytes;

	bool is_indexed() { return (bool)this->bits.get_mask(MASK_IS_INDEXED); }
	void set_is_indexed(bool new_indexed)
	{
	    this->bits.set_mask((u32)new_indexed, MASK_IS_INDEXED);
	}
	u8 idx_stride() { return (u8)this->bits.get_mask(MASK_IDX_STRIDE); }
	void set_idx_stride(u8 new_idx_stride)
	{
	    this->bits.set_mask(new_idx_stride, MASK_IDX_STRIDE);
	}
};

} // rg

#endif // _RG_GEOMETRY_HPP_ 
