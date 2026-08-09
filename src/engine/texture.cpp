#define STB_IMAGE_IMPLEMENTATION

#include "core/basic.hpp"
#include "collections/thread_pool.hpp"
#include "core/context.hpp"
#include "engine/gltf.hpp"
#include "engine/texture.hpp"
#include "engine/stb_image.h"
#include "engine/vk_core.hpp"
#include "engine/entry.hpp"

namespace rg
{

intern Maybe<u8*> load_from_disk(Texture* self);
intern s32 texture_load_cpu_and_transfer_gpu_task(void* arg);

bool Texture::load_cpu(
	const Path& texture_path,
	VulkanContext* vk_ctx,
	VulkanBufferCpu* staging_buff,
	GltfSampler gltf_sampler
)
{
	// Move path memory.
	this->path = texture_path;

	ASSERT_MSG(this->load_state < TextureLoadState::LOADED_CPU_VISIBLE, "Texture init: it shouldn't be cpu loaded already, path: " FMT_PLACEHOLDER_LEN, FMT_DSTRING_VAL(this->path));

	{
		Arena* talloc = get_temp_allocator();
		TEMP_ALLOC_SCOPE(talloc);

		auto [texture_data, is_ok] = load_from_disk(this);
		if (!is_ok)
		{
			this->load_state = TextureLoadState::FREE;
			return false;
		}

		Array<Slice<u8>, 1> data_to_append = {{ texture_data, this->size }};
		EngineContext* engine_ctx = get_engine_context();

		engine_ctx->mutex.lock();
		this->buff_view = staging_buff->append_data(data_to_append.slice());
		engine_ctx->mutex.unlock();
	}

	this->load_state = TextureLoadState::LOADED_CPU_VISIBLE;

	this->image.init(
		vk_ctx,
		VK_FORMAT_B8G8R8A8_UNORM,
		this->width,
		this->height,
		1,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
	);

	this->gltf_sampler = gltf_sampler;

	// NOTE: DEBUG
	LOG_INFO("Loaded cpu texture: " FMT_PLACEHOLDER_LEN, FMT_DSTRING_VAL(this->path));
	return true;
}

intern Maybe<u8*> load_from_disk(Texture* self)
{
	Maybe<u8*> res;

	u8* data = stbi_load(
		(CString)self->path.cstr(),
		(s32*)&self->width,
		(s32*)&self->height,
		(s32*)&self->channels,
		TEXTURE_CHANNEL_COUNT
	);

	if (!data)
	{
		LOG_ERROR("Error loading texture: file_path: " FMT_PLACEHOLDER_LEN ", reason: %s",
			FMT_DSTRING_VAL(self->path), stbi_failure_reason());
		stbi__err("", "");
		return res;
	}

	self->size = self->width * TEXTURE_CHANNEL_COUNT * self->height;
	res.set_val(data);
	return res;
}

void Texture::cmd_transfer_to_gpu(
	VulkanCmdBuffer cmd_buffer,
	VulkanContext* vk_ctx,
	VulkanBufferCpu* staging_buff
)
{
	this->image.cmd_transition_layout(
		cmd_buffer,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		VK_ACCESS_TRANSFER_WRITE_BIT
	);

	this->image.copy_data(cmd_buffer, *staging_buff, this->buff_view, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	this->image.cmd_transition_layout(
		cmd_buffer,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT
	);

	VkSamplerCreateInfo sampler_ci = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = GLTF_MAG_FILTERS[this->gltf_sampler.mag_filter].vk_filter,
		.minFilter = GLTF_MIN_FILTERS[this->gltf_sampler.min_filter].vk_filter,
		.mipmapMode = GLTF_MIN_FILTERS[this->gltf_sampler.min_filter].mipmap_mode,
		.addressModeU = GLTF_WRAP_KINDS[this->gltf_sampler.wrap_s].vk_address_mode,
		.addressModeV = GLTF_WRAP_KINDS[this->gltf_sampler.wrap_t].vk_address_mode,
		.addressModeW = GLTF_WRAP_KINDS[this->gltf_sampler.wrap_s].vk_address_mode,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = 16,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
	};

	VK_CHECK(vkCreateSampler(vk_ctx->dev.log_dev, &sampler_ci, vk_ctx->vk_alloc, &this->vk_sampler));

	this->load_state = TextureLoadState::LOADED_GPU;

	// NOTE: DEBUG
	LOG_INFO("Transfered gpu texture: " FMT_PLACEHOLDER_LEN, FMT_DSTRING_VAL(this->path));
}

bool Texture::load_cpu_and_transfer_gpu(
	const Path& texture_path,
	VulkanContext* vk_ctx,
	VulkanBufferCpu* staging_buff,
	GltfSampler gltf_sampler,
	VulkanCmdBuffer cmd_buffer
)
{
	if (!this->load_cpu(texture_path, vk_ctx, staging_buff, gltf_sampler)) return false;
	this->cmd_transfer_to_gpu(cmd_buffer, vk_ctx, staging_buff);
	return true;
}

intern s32 texture_load_cpu_and_transfer_gpu_task(void* arg)
{
	auto* data = (TextureUploadTask*)arg;
	if (!data->self->load_cpu(
		*data->texture_path,
		data->vk_ctx,
		data->staging_buff,
		data->gltf_sampler
	)) return 0;
	data->self->cmd_transfer_to_gpu(data->cmd_buffer, data->vk_ctx, data->staging_buff);
	return 1;
}

void Texture::destroy(VulkanContext* ctx)
{
	ctx->dev.wait_idle();
	this->image.destroy(ctx);

	if (this->vk_sampler)
	{
		vkDestroySampler(ctx->dev.log_dev, this->vk_sampler, ctx->vk_alloc);
		this->vk_sampler = null;
	}

	this->load_state = TextureLoadState::FREE;
}

VkDescriptorImageInfo Texture::to_vk_descriptor_info()
{
	return {
		.sampler = this->vk_sampler,
		.imageView = this->image.view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};
}

// Texture System.

void TextureSystem::init(Allocator* alloc, sz capacity)
{
	this->textures.init(alloc, capacity);
	for (auto& tex : this->textures)
	{
		tex.load_state = TextureLoadState::FREE;
	}
	this->lookup_table.init(alloc, capacity, TABLE_LOAD_FACTOR);
	EngineContext* engine_ctx = get_engine_context();
	this->staging_buff.init(
		&engine_ctx->vk_ctx,
		INIT_TEX_STAGING_BUFF_CAPACITY,
		{
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.sharing_mode = VK_SHARING_MODE_EXCLUSIVE,
			.mem_props = (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
		}
	);
}

void TextureSystem::load_new_textures_cpu_and_transfer_to_gpu(
	Slice<TextureCreateConfig> configs,
	Slice<VulkanCmdBuffer> cmd_buffs
)
{
	BENCH_SCOPE(b, "Texture loading");
	
	EngineContext* engine_ctx = get_engine_context();
	Arena* talloc = get_temp_allocator();
	TEMP_ALLOC_SCOPE(talloc);

	auto tasks = Slice<TextureUploadTask>::make(talloc, configs.count);
	auto thread_tasks = Slice<ThreadTask>::make(talloc, configs.count);

	TextureUploadTask* curr_task = tasks.ptr;
	TextureUploadTask* tasks_end = tasks.end();
	ThreadTask* curr_thread_task = thread_tasks.ptr;
	TextureCreateConfig* curr_config = configs.ptr;
	sz cmd_buff_idx = 0; 
	VulkanCmdBuffer curr_cmd_buff;

	for (; curr_task != tasks_end;)
	{
		curr_cmd_buff = cmd_buffs[cmd_buff_idx];

		auto [tex, idx] = this->textures.get_free_slot_and_idx();
		curr_config->idx = idx;
		curr_task->self = tex;
		curr_task->texture_path = &curr_config->path;
		curr_task->vk_ctx = &engine_ctx->vk_ctx;
		curr_task->staging_buff = &this->staging_buff;
		curr_task->gltf_sampler = curr_config->gltf_sampler;
		curr_task->cmd_buffer = curr_cmd_buff;

		curr_thread_task->arg = curr_task;
		curr_thread_task->fn = texture_load_cpu_and_transfer_gpu_task;

		++curr_task;
		++curr_config;
		++curr_thread_task;
		// NOTE: its somehow possible to record a single buffer...
		// wrap_inc_assume_pow_two(cmd_buff_idx, cmd_buffs.count);
	}

	engine_ctx->thread_pool.submit_task_many(thread_tasks);
	// TODO: wait from IO thread.
	engine_ctx->thread_pool.await();
}

inline StrView path_to_key(const Path& path)
{
	return path.get_view_without_extension();
}

Maybe<Texture*> TextureSystem::get_texture_by_path(const Path& path)
{
	Maybe<Texture*> res;
	auto [idx, found] = this->lookup_table.get(path_to_key(path));
	if (!found) return res;
	res.set_val(this->get_texture_by_idx(*idx));
	return res;
}

void TextureSystem::remove_texture(TextureIdx idx)
{
	Texture* tex = &this->textures[idx];
	(void)this->lookup_table.remove(path_to_key(tex->path));
	tex->load_state = TextureLoadState::FREE;
}

void TextureSystem::destroy()
{
	this->textures.destroy();
	this->lookup_table.destroy();
}

} // rg

// Stb allocation functions.

using namespace rg;

void* stb_alloc_extern(sz size)
{
	// LOG_TRACE("STb alloc of size: %dl", size);
    Arena* talloc = get_temp_allocator(); 
    return allocator_allocate(talloc, size);
}

void* stb_realloc_extern(void* old_ptr, sz new_size)
{
	// LOG_TRACE("STb realloc of size: %dl on ptr: %p", new_size, old_ptr);
    Arena* talloc = get_temp_allocator(); 
    return allocator_reallocate(talloc, old_ptr, new_size);
}

// Made as noop because we rely on arena 'marks' to free memory.
void  stb_free_extern(void* ptr)
{     
	// LOG_TRACE("Stb free on ptr: %p", ptr);
}
