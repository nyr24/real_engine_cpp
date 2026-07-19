#ifndef _RG_ENTITY_HPP_
#define _RG_ENTITY_HPP_

#include "core/basic.hpp"
#include "core/bits.hpp"
#include "engine/geometry.hpp"

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

enum struct EntityTextureKind
{
	DIFFUSE,
	NORMAL,
	OCCLUSSION,
	EMISSIVE,
	METALLIC_ROUGHNESS,
};

// Shader binding order.
alias EntityTextureBinding = EntityTextureKind;

struct EntityTransforms
{
	union
	{
		struct
		{
			// TODO: quaternions
			// Quat rotation;
			Vec3 scale;
			Vec3 translation;
		};
		Mat4 matrix;
	};
};

// Not used in Entity, for space efficiency reasons.
// Used in entity initialization for the ease of caller.
struct EntityTransformsTagged
{
	union
	{
		struct
		{
			// Quat rotation;
			Vec3 scale;
			Vec3 translation;
		};
		Mat4 matrix;
	};
	bool has_matrix;
};

alias TextureIdx = s32;

struct Entity
{
	GeometryView geometry;
	EntityShaderState shader_state;
	EntityTransforms transforms;
	// // There are 2 ways of storing textures:
	// // 1. - store indices - less memory usage. (choosen)
	// // 2. - store pointers - direct references - don't need to lookup stuff (but lookup into arr is fast).
	struct
	{
		TextureIdx diffuse_tex;
		TextureIdx normal_tex;
		TextureIdx occlussion_tex;
		TextureIdx emissive_tex;
		TextureIdx metallic_roughness_tex;
	};
};
 
} // rg

#endif // _RG_ENTITY_HPP_
