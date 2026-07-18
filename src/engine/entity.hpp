#ifndef _RG_ENTITY_HPP_
#define _RG_ENTITY_HPP_

#include "core/basic.hpp"
#include "core/bits.hpp"

namespace rg
{

struct EntityShaderState
{
	u16 pool_idx;
	u16 sets_count;

	// mbs[sets_start_idx=31..has_matrix=1]lsb
	BitInt<u32> bits;
	bool has_matrix() { return this->bits.get_mask(BIT_MASK_32_LOW_1); }
	void set_has_matrix(bool new_has_matrix)
	{
	    this->bits.set_mask((u32)new_has_matrix, BIT_MASK_32_LOW_1);
	}
	u32 sets_start_idx() { return this->bits.get_mask(~BIT_MASK_32_LOW_1); }
	void set_sets_start_idx(u32 new_idx)
	{
	    this->bits.set_mask(new_idx, ~BIT_MASK_32_LOW_1); 
	}
};

   
} // rg

#endif // _RG_ENTITY_HPP_
