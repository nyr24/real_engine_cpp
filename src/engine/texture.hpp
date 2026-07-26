#ifndef _RG_TEXTURE_HPP_
#define _RG_TEXTURE_HPP_

#include "volk/volk.h"
#include "engine/shared.hpp"
#include "core/basic.hpp"
#include "core/io.hpp"
#include "collections/slot_array.hpp"
#include "collections/hashmap.hpp"
#include "engine/vk_core.hpp"
#include "engine/gltf.hpp"

namespace rg
{

constexpr sz MAX_TEXTURE_NAME_LEN  = 32;
constexpr sz TEXTURE_CHANNEL_COUNT = 4;
constexpr TextureIdx TEX_INDEX_INVALID = -1;
constexpr f32 TABLE_LOAD_FACTOR = 0.95;
constexpr sz INIT_TEX_STAGING_BUFF_CAPACITY = 1 << 28;

enum struct TextureLoadState
{
	FREE,
	SLOT_TAKEN,
	LOADED_CPU_VISIBLE,
	LOADED_GPU,
	EnumSize
};

struct Texture
{
	TextureLoadState load_state;
	u32 width;
	u32 height;
	u32 size;
	u32 channels;
	VulkanImage image;
	BufferChunkView buff_view;
	GltfSampler gltf_sampler;
	VkSampler vk_sampler;
	Path path;

	bool load_cpu(
		const Path& texture_path,
		VulkanContext* vk_ctx,
		VulkanBufferCpu* staging_buff,
		GltfSampler gltf_sampler
	);
	void cmd_transfer_to_gpu(
		VulkanCmdBuffer cmd_buffer,
		VulkanContext* vk_ctx,
		VulkanBufferCpu* staging_buff
	);
	bool load_cpu_and_transfer_gpu(
		const Path& texture_path,
		VulkanContext* vk_ctx,
		VulkanBufferCpu* staging_buff,
		GltfSampler gltf_sampler,
		VulkanCmdBuffer cmd_buffer
	);
	VkDescriptorImageInfo to_vk_descriptor_info();
	void destroy(VulkanContext* ctx);
};

struct TextureUploadTask
{
	Texture* self;
	Path* texture_path;
	VulkanContext* vk_ctx;
	VulkanBufferCpu* staging_buff;
	GltfSampler gltf_sampler;
	VulkanCmdBuffer cmd_buffer;
};

// Texture system.

struct TextureSystemSlot
{
	Texture* texture;
	TextureIdx idx;
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

struct TextureLookupState
{
	TextureIdx idx;
	TextureLoadState load_state;
};

struct TextureLookupResult
{
	Slice<TextureLookupState> states;
	EnumArray<u32, TextureLoadState> counts;
};

struct TextureSystem
{
	static constexpr sz DEFAULT_CAPACITY = 128;

	SlotArray<Texture> textures;
	HashMap<StrView, TextureIdx> lookup_table;
	VulkanBufferCpu staging_buff;

	void init(Allocator* alloc, sz init_capacity = DEFAULT_CAPACITY);
	// All textures are required to be new (not allocated on cpu / gpu) for this call.
	void load_new_textures_cpu_and_transfer_to_gpu(
		Slice<TextureCreateConfig> configs,
		Slice<VulkanCmdBuffer> cmd_buffs
	);
	Maybe<Texture*> get_texture_by_path(const Path& path);
	void remove_texture(TextureIdx idx);
	void destroy();

	Texture* get_texture_by_idx(TextureIdx idx)
	{
		return &this->textures[idx];
	}
};

} // rg

#endif // _RG_TEXTURE_HPP_ 
