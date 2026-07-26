#ifndef _RG_GLTF_HPP_
#define _RG_GLTF_HPP_

#include "volk/volk.h"
#include "collections/farray.hpp"

namespace rg
{
 
enum struct GltfSamplerMagFilter
{
	NEAREST,
	LINEAR,
	EnumSize
};

struct GltfSamplerMagFilterAssoc
{
	u32 gltf_val;
	VkFilter vk_filter;
	VkSamplerMipmapMode mipmap_mode;
};

constexpr EnumArray<GltfSamplerMagFilterAssoc, GltfSamplerMagFilter> GLTF_MAG_FILTERS = {
	{ 9728, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST },
	{ 9729, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR },
};

enum struct GltfSamplerMinFilter
{
	NEAREST,
	LINEAR,
	NEAREST_MIPMAP_NEAREST,
	LINEAR_MIPMAP_NEAREST,
	NEAREST_MIPMAP_LINEAR,
	LINEAR_MIPMAP_LINEAR,
	EnumSize
};

struct GltfSamplerMinFilterAssoc
{
	u32 gltf_val;
	VkFilter vk_filter;
	VkSamplerMipmapMode mipmap_mode;
};

constexpr EnumArray<GltfSamplerMinFilterAssoc, GltfSamplerMinFilter> GLTF_MIN_FILTERS = {
	{ 9728, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST },
	{ 9729, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR },
	{ 9984, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST },
	{ 9985, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST },
	{ 9986, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_LINEAR },
	{ 9987, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR },
};

enum struct GltfSamplerWrapKind
// : ushort (uint gltf_val, VkSamplerAddressMode vk_address_mode)
{
	CLAMP_TO_EDGE,
	MIRRORED_REPEAT,
	REPEAT,
	EnumSize
};

struct GltfSamplerWrapKindAssoc
{
	u32 gltf_val;
	VkSamplerAddressMode vk_address_mode;
};

constexpr EnumArray<GltfSamplerWrapKindAssoc, GltfSamplerWrapKind> GLTF_WRAP_KINDS = {
	{ 33071, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE },
	{ 33648, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT },
	{ 10497, VK_SAMPLER_ADDRESS_MODE_REPEAT },
};

struct GltfSampler
{
	GltfSamplerMagFilter mag_filter;
	GltfSamplerMinFilter min_filter;
	GltfSamplerWrapKind wrap_s;
	GltfSamplerWrapKind wrap_t;
};

} // rg

#endif // _RG_GLTF_HPP_
