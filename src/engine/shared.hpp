#ifndef _RG_SHARED_HPP_
#define _RG_SHARED_HPP_

#include "core/basic.hpp"
#include "core/io.hpp"
#include "core/math.hpp"
#include "collections/bits.hpp"
#include "engine/gltf.hpp"

namespace rg
{

alias TextureIdx = s32;
alias ColorRGB = Vec3;
alias ColorRGBA = Vec4;

enum IndexStride : u8
{
	INDEX_STRIDE_CHAR = 1,
	INDEX_STRIDE_SHORT = 2,
	INDEX_STRIDE_INT = 4,
};

enum struct EntityTextureKind
{
	DIFFUSE,
	NORMAL,
	OCCLUSSION,
	EMISSIVE,
	METALLIC_ROUGHNESS,
	EnumSize
};

constexpr sz MAX_ENTITY_TEX_COUNT = (sz)EntityTextureKind::EnumSize;

struct EntityShaderState
{
	u16 pool_idx;
	u16 sets_count;
	u32 start_idx;
	// mbs[sets_start_idx=31..has_matrix=1]lsb
	// BitInt<u32> bits;

	// bool has_matrix() { return this->bits.get_mask(BIT_MASK_32_LOW_1); }
	// void set_has_matrix(bool new_has_matrix)
	// {
	//     this->bits.set_mask((u32)new_has_matrix, BIT_MASK_32_LOW_1);
	// }
	// u32 sets_start_idx() { return this->bits.get_mask(~BIT_MASK_32_LOW_1); }
	// void set_sets_start_idx(u32 new_idx)
	// {
	//     this->bits.set_mask(new_idx, ~BIT_MASK_32_LOW_1); 
	// }
};

enum struct TransformType
{
	SCALE,
	TRANSLATION,
	ROTATION,
};

struct TextureCreateConfig
{
	Path path;
	GltfSampler gltf_sampler;
	// Result index.
	TextureIdx idx;
	// Passed from outside if needed.
	EntityTextureKind kind;
};

const ColorRGB COLOR_WHITE_RGB      = { 1.0f, 1.0f, 1.0f };
const ColorRGB COLOR_BLACK_RGB      = { 0.0f, 0.0f, 0.0f };
const ColorRGB COLOR_RED_RGB        = { 1.0f, 0.0f, 0.0f };
const ColorRGB COLOR_GREEN_RGB      = { 0.0f, 1.0f, 0.0f };
const ColorRGB COLOR_BLUE_RGB       = { 0.0f, 0.0f, 1.0f };
const ColorRGB COLOR_ORANGE_RGB     = { 0.7f, 0.3f, 0.0f };
const ColorRGB COLOR_PURPLE_RGB     = { 0.5f, 0.0f, 0.5f };
const ColorRGB COLOR_YELLOW_RGB     = { 0.5f, 0.5f, 0.0f };

const ColorRGBA COLOR_WHITE_RGBA    = { 1.0f, 1.0f, 1.0f, 1.0f };
const ColorRGBA COLOR_BLACK_RGBA    = { 0.0f, 0.0f, 0.0f, 1.0f };
const ColorRGBA COLOR_RED_RGBA      = { 1.0f, 0.0f, 0.0f, 1.0f };
const ColorRGBA COLOR_GREEN_RGBA    = { 0.0f, 1.0f, 0.0f, 1.0f };
const ColorRGBA COLOR_BLUE_RGBA     = { 0.0f, 0.0f, 1.0f, 1.0f };
const ColorRGBA COLOR_ORANGE_RGBA   = { 0.0f, 0.5f, 0.1f, 1.0f };
const ColorRGBA COLOR_PURPLE_RGBA   = { 0.04f, 0.0f, 0.1f, 1.0f };
const ColorRGBA COLOR_YELLOW_RGBA   = { 0.0f, 1.0f, 0.0f, 1.0f };
const ColorRGBA COLOR_DARK_BLUE_RGBA= { 0.0f, 0.0f, 0.15f, 1.0f };

// ---- Cube geometry ----

constexpr sz CUBE_VERT_COUNT = 24;
constexpr sz CUBE_INDEX_COUNT = 36;

// Positions
const Vec3 POS_TLN = {-0.5f, -0.5f, +0.0f};
const Vec3 POS_TLF = {-0.5f, -0.5f, -1.0f};
const Vec3 POS_TRN = {+0.5f, -0.5f, +0.0f};
const Vec3 POS_TRF = {+0.5f, -0.5f, -1.0f};
const Vec3 POS_BLN = {-0.5f, +0.5f, +0.0f};
const Vec3 POS_BLF = {-0.5f, +0.5f, -1.0f};
const Vec3 POS_BRN = {+0.5f, +0.5f, +0.0f};
const Vec3 POS_BRF = {+0.5f, +0.5f, -1.0f};

// Texture positions
const Vec2 TEX_COORD_TL = {+0.0f, +1.0f};
const Vec2 TEX_COORD_TR = {+1.0f, +1.0f};
const Vec2 TEX_COORD_BL = {+0.0f, +0.0f};
const Vec2 TEX_COORD_BR = {+1.0f, +0.0f};

// Normals
const Vec3 NORMAL_UP       = {+0.0f, +1.0f, +0.0f};
const Vec3 NORMAL_DOWN     = {+0.0f, -1.0f, +0.0f};
const Vec3 NORMAL_RIGHT    = {+1.0f, +0.0f, +0.0f};
const Vec3 NORMAL_LEFT     = {-1.0f, +0.0f, +0.0f};
const Vec3 NORMAL_FORWARD  = {+0.0f, +0.0f, +1.0f};
const Vec3 NORMAL_BACKWARD = {+0.0f, +0.0f, -1.0f};

} // rg

#endif // _RG_SHARED_HPP_ 
