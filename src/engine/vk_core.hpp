#ifndef _RG_VULKAN_CORE_HPP_
#define _RG_VULKAN_CORE_HPP_

#include <vulkan/vulkan.h>
#include "collections/farray.hpp"
#include "collections/darray.hpp"
#include "collections/string.hpp"
#include "core/bits.hpp"
#include "core/thread.hpp"
#include "engine/entity.hpp"

namespace rg
{

struct VulkanContext;
struct VulkanDevice;
bool map_vk_mem(VulkanDevice* dev, void** dest, VkDeviceMemory src, sz size_bytes, sz offset = 0, VkMemoryMapFlags flags = 0);
void unmap_vk_mem(VulkanDevice* dev, VkDeviceMemory gpu_mem);

// Vulkan synch primitives.

struct VulkanSemaphore
{
	VkSemaphore handle;

    void init(VulkanContext* ctx);
	void destroy(VulkanContext* ctx);
};

struct VulkanFence
{
    VkFence handle;
    bool    is_signaled;

	void init(VulkanContext* ctx, bool init_signaled = false);
    bool wait(VulkanContext* ctx);
    void reset(VulkanContext* ctx);
	void destroy(VulkanContext* ctx);
};

// Buffers.

struct VulkanBufferConfig
{
	VkBufferUsageFlagBits usage;
	VkSharingMode sharing_mode;
	VkMemoryPropertyFlagBits mem_props;
};

struct BufferChunkView
{
	sz offset;
	sz size;
};

struct VulkanBuffer
{
	VkBuffer handle;
	sz capacity;
	VkDeviceMemory gpu_mem;
	VulkanBufferConfig config;

	void init(VulkanContext* ctx, sz capacity, const VulkanBufferConfig& config);
	void destroy(VulkanContext* ctx);
	void realloc(sz capacity);

	bool buffer_is_initialized(VulkanBuffer* self)
	{
		return this->handle && this->gpu_mem;
	}
};

struct VulkanBufferCpu : VulkanBuffer
{
	sz size;
	void* cpu_mem;
	Mutex mutex;

	void init(VulkanContext* ctx, sz capacity, const VulkanBufferConfig& config);
	void destroy(VulkanContext* ctx);
	BufferChunkView append_data(Slice<Slice<u8>> data);
	Maybe<BufferChunkView> append_data_non_realloc(Slice<Slice<u8>> data);

	bool buffer_cpu_is_initialized()
	{
		return this->handle && this->cpu_mem && this->gpu_mem;
	}
	sz remain_mem() { return this->capacity - this->size; }
	BufferChunkView full_view() { return { 0, this->size }; }
	void reset() { this->size = 0; }
};

// Command buffers.

// State of the command buffer.
// stored separately to eliminate padding.
enum struct CmdBufferState
{
	NOT_ALLOCATED,
	READY,
	RECORDING_BEGIN,
	RECORDING_END,
	SUBMITTED,
};

struct VulkanCmdBuffer
{
	VkCommandBuffer handle;

	void begin_recording(
		CmdBufferState* state,
		VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	);
	void end_recording(CmdBufferState* state);
	void submit(
		CmdBufferState* cmd_buff_state,
		VulkanContext* ctx,
		VkQueue queue,
		Slice<VulkanSemaphore> signal_semaphores = {},
		Slice<VulkanSemaphore> wait_semaphores = {},
		Slice<VkPipelineStageFlags> wait_dst_stage_masks = {},
		Maybe<VulkanFence> mb_fence = Maybe<VulkanFence>{}
	);
	void reset(CmdBufferState* cmd_buff_state, VkCommandBufferResetFlags flags = 0);
};

void cmd_buffer_begin_recording_many(
	Slice<VulkanCmdBuffer> buffers,
	Slice<CmdBufferState> states,
	VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
);

void cmd_buffer_end_recording_many(
	Slice<VulkanCmdBuffer> buffers,
	Slice<CmdBufferState> states
);

void cmd_buffer_submit_many(
	Slice<VulkanCmdBuffer> cmd_buffs,
	Slice<CmdBufferState> cmd_buff_states,
	VulkanContext* ctx,
	VkQueue queue,
	Slice<VulkanSemaphore> signal_semaphores = {},
	Slice<VulkanSemaphore> wait_semaphores = {},
	Slice<VkPipelineStageFlags> wait_dst_stage_masks = {},
	Maybe<VulkanFence> mb_fence = Maybe<VulkanFence>{}
);

void cmd_buffer_reset_many(
	Slice<VulkanCmdBuffer> cmd_buffs,
	Slice<CmdBufferState> cmd_buff_states,
	VkCommandBufferResetFlags flags = 0
);

void cmd_buffer_end_submit_reset_many(
	Slice<VulkanCmdBuffer> cmd_buffs,
	Slice<CmdBufferState> cmd_buff_states,
	VulkanContext* vk_ctx,
	VkQueue queue,
	bool wait_on_queue = false
);

// Command pool.

struct VulkanCmdPool
{
	VkCommandPool handle;

	void init(VulkanContext* ctx, u8 queue_fam_idx);
	void allocate_cmd_buffers(
		VulkanDevice* dev,
		Slice<VulkanCmdBuffer> out_cmd_buffs,
		bool is_primary = true
	);
	void destroy_cmd_buffers(VulkanDevice* dev, Slice<VulkanCmdBuffer> out_cmd_buffs);
	void destroy(VulkanContext* ctx);
};

// Image and ImageView.

struct VulkanImage
{
	VkImage handle;
	VkImageView view;
	VkDeviceMemory memory;
	u32 width;
	u32 height;
	u32 depth;
	VkImageType type;
	VkFormat format;
	VkImageAspectFlags aspect_flags;

	void init(
		VulkanContext* ctx,
		VkFormat format,
		u32 width,
		u32 height,
		u32 depth = 1,
		VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT,
		VkImageLayout init_layout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkMemoryPropertyFlagBits mem_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VkImageType type = VK_IMAGE_TYPE_2D,
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
		VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT,
		bool should_create_view = true
	);
	void create_view(VulkanContext* ctx, VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT);
	void cmd_transition_layout(
		VulkanCmdBuffer cmd_buff,
		VkImageLayout src_layout,
		VkImageLayout dest_layout,
		VkPipelineStageFlagBits2 src_stage_mask,
		VkPipelineStageFlagBits2 dest_stage_mask,
		VkAccessFlagBits2 src_access_mask,
		VkAccessFlagBits2 dest_access_mask
	);
	void copy_data(
		VulkanCmdBuffer cmd_buff,
		const VulkanBufferCpu& src_buff,
		BufferChunkView buff_view,
		VkImageLayout dst_image_layout
	);
	void destroy(VulkanContext* ctx);
};

VkImageView create_view_from_raw_image(
	VulkanContext* ctx,
	VkImage raw_image,
	VkFormat format,
	VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D,
	VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT
);

void cmd_transition_layout_raw_image(
	VkImage raw_image,
	VulkanCmdBuffer cmd_buff,
	VkImageLayout src_layout,
	VkImageLayout dest_layout,
	VkPipelineStageFlagBits2 src_stage_mask,
	VkPipelineStageFlagBits2 dest_stage_mask,
	VkAccessFlagBits2 src_access_mask,
	VkAccessFlagBits2 dest_access_mask,
	bool is_depth
);

// Shaders and pipeline.

constexpr sz MAX_PIPELINE_COUNT            = 5;
constexpr sz MAX_ATTRIB_COUNT              = 10;
constexpr sz MAX_DESCRIPTOR_BINDING_COUNT  = 10;
constexpr sz MAX_DESCRIPTOR_LAYOUT_COUNT   = 10;
constexpr sz MAX_DESCRIPTOR_POOL_COUNT     = 10;
constexpr sz MAX_PUSH_CONSTANT_RANGE_COUNT = 5;
constexpr sz INIT_POOL_COUNT               = 4;

// VulkanShader & VulkanPipeline

struct VulkanDescriptorSetLayout
{
	VkDescriptorSetLayout handle;
	FArray<VkDescriptorSetLayoutBinding, MAX_DESCRIPTOR_BINDING_COUNT> bindings;

	void init(VulkanContext* ctx, Slice<VkDescriptorSetLayoutBinding> bindings);
};

VulkanDescriptorSetLayout create_descriptor_set_layout(VulkanContext* ctx, Slice<VkDescriptorSetLayoutBinding> bindings);
void init_descriptor_bindings(
	VkShaderStageFlags stage_flags,
	Slice<VkDescriptorType> descriptor_types,
	Slice<VkDescriptorSetLayoutBinding> out_bindings
);

struct PoolAllocationResult
{
	sz start_index;
	sz count;
};

// Knows max_sets from shader,
// can't allocate over max_sets.
struct VulkanDescriptorPool
{
	VkDescriptorPool handle;
	DArray<VkDescriptorSet> sets;

	void init(VulkanContext* ctx, Slice<VkDescriptorPoolSize> pool_sizes, u32 max_sets);
	Maybe<PoolAllocationResult> allocate_sets(VulkanDevice* dev, Slice<VkDescriptorSetLayout> layouts);
	Slice<VkDescriptorSet> get_sets(sz index, sz len);
	void free_sets(VulkanContext* ctx, uint start, ushort len);
};

VulkanDescriptorPool create_descriptor_pool(VulkanContext* ctx, Slice<VkDescriptorPoolSize> pool_sizes, u32 max_sets);

enum struct ShaderStageKind : u8
{
	VERTEX,
	FRAGMENT,
	COMPUTE,
	GEOMETRY,
	TESSELLATION_CONTROL,
	TESSELLATION_EVAL,
	EnumSize
};

struct VulkanShaderStage
{
	CString entry_name;
	ShaderStageKind kind;
};

struct VulkanShaderModule
{
	VkShaderModule handle;
	BitInt<u8> stage_bits;

	bool init(
		VulkanContext* ctx,
		StrView file_name,
		BitInt<u8> stage_bits
	);
	void get_pipeline_stages(
		FArray<VkPipelineShaderStageCreateInfo, (sz)ShaderStageKind::EnumSize>* out_pipeline_stages
	);
};

struct DescriptorSetInfo
{
	Slice<VkDescriptorSetLayoutBinding> bindings;
	Slice<VkDescriptorType> types;
};

struct DescriptorUpdateInfo
{
	u32 set_index;
	VkDescriptorType type;
	union
	{
		VkDescriptorImageInfo image_info;
		VkDescriptorBufferInfo buff_info;
		VkBufferView* texel_buff_info;
	};
};

struct VulkanPipelineConfig
{
	VkPipelineBindPoint bind_point;
	VkPrimitiveTopology primitive_topology;
	VkPolygonMode polygon_mode;
	f32 line_width;
	VkSampleCountFlagBits sample_count;
	bool enable_color_blend;
};

struct VulkanShaderConfig
{
	Slice<DescriptorSetInfo> descriptor_set_infos;
	Slice<VkVertexInputAttributeDescription> attrib_descriptions;
	Slice<VulkanPipelineConfig> pipeline_configs;
	Slice<VkPushConstantRange> push_constant_ranges;
	u32 max_entities_per_frame;
	BitInt<u8> stage_bits;
	u8 vertex_binding_index;
};

struct VulkanPipeline
{
	VkPipeline handle;
	VkPipelineBindPoint bind_point;
	VkPipelineLayout layout;

	void cmd_bind(VulkanCmdBuffer cmd_buffer);
};

struct VulkanShader
{
	VulkanContext* ctx;
	FArray<VulkanPipeline, MAX_PIPELINE_COUNT> pipelines;
	FArray<VulkanDescriptorSetLayout, MAX_DESCRIPTOR_LAYOUT_COUNT> descriptor_layouts;
	// for creating new pools easily
	FArray<VkDescriptorPoolSize, MAX_DESCRIPTOR_BINDING_COUNT> pool_sizes;
	DArray<VulkanDescriptorPool> descriptor_pools;
	VulkanShaderModule mod;
	u32 curr_pipeline;
	u32 max_entities;
	BitInt<u8> stage_bits;

	bool init(VulkanContext* ctx, StrView file_name, VulkanShaderConfig* config);
	void init_descriptor_state(VulkanContext* ctx, VulkanShaderConfig* config);
	void create_pipeline(
		VulkanPipeline* out_pipeline,
		VulkanContext* ctx,
		VulkanPipelineConfig* config,
		Slice<VkVertexInputAttributeDescription> attrib_descriptions,
		u8 vertex_bind_ind
	);
	VulkanDescriptorPool* allocate_pool(VulkanContext* ctx);
	VulkanPipeline* get_curr_pipeline()
	{
		return &this->pipelines[this->curr_pipeline];
	}
	void cmd_bind_curr_pipeline(VulkanCmdBuffer cmd_buffer);
	EntityShaderState allocate_entity_resources(VulkanContext* ctx);
	Slice<VkDescriptorSet> get_entity_resources(EntityShaderState entity_state, sz curr_frame);
	void update_entity_resources(
		VulkanContext* ctx,
		EntityShaderState entity_state,
		Slice<DescriptorUpdateInfo> update_infos,
		sz curr_frame
	);
	void cmd_bind_entity_resources(
		VulkanCmdBuffer cmd_buff,
		EntityShaderState entity_state,
		VulkanPipeline* pipeline,
		sz curr_frame,
		sz first_set = 0
	);
	void destroy_entity_resources(VulkanContext* ctx, EntityShaderState entity_state);
};

// Swapchain.

struct SwapchainSupportInfo
{
	static constexpr sz MAX_FORMATS = 128;
	static constexpr sz MAX_PRESENT_MODES = 8;

	FArray<VkSurfaceFormatKHR, MAX_FORMATS> formats;
	FArray<VkPresentModeKHR, MAX_PRESENT_MODES> present_modes;
	VkSurfaceCapabilitiesKHR capabilities;
};

enum struct SwapchainPresentResult
{
	SUCCESS,
	NEEDS_RECREATION,
	NEEDS_RECREATION_CANT_PROCEED,
};

struct VulkanSwapchain
{
	static constexpr sz MAX_SWAPCHAIN_IMAGE_COUNT = 5;

	FArray<VkImage, MAX_SWAPCHAIN_IMAGE_COUNT> images;
	FArray<VkImageView, MAX_SWAPCHAIN_IMAGE_COUNT> image_views;
	VulkanImage depth_image;
	VkSwapchainKHR handle;
	VkPresentModeKHR present_mode;
	VkSurfaceFormatKHR surface_fmt;
	VkSharingMode sharing_mode;
	BitInt<u32> bits;
	u32 curr_image_index;
	bool recreation_scheduled;

	bool init(VulkanContext* ctx, VkExtent2D extent);
	bool recreate(VulkanContext* ctx, VkExtent2D extent);
	void recreate_if_needed(VulkanContext* ctx);
	SwapchainPresentResult acquire_next_image_index(
		VulkanContext* ctx,
		VulkanSemaphore image_available_semaphore,
		u64 timeout_ns = 0
	);
	SwapchainPresentResult present(
		VulkanContext* ctx,
		VkQueue present_queue,
		VulkanSemaphore render_complete_semaphore
	);
	bool acquire_format_and_present_mode(SwapchainSupportInfo* supp_info);
	void destroy(VulkanContext* ctx);

	VkImage get_curr_image()
	{
		return this->images[this->curr_image_index];
	}

	VkImageView get_curr_view()
	{
		return this->image_views[this->curr_image_index];
	}
};

// Queue.

struct QueueIndex
{
	u8 fam;
	u8 queue;
};

struct QueueCount
{
	u8 fam_ind;
	u8 count;
};

struct QueueIndices
{
	QueueIndex graphics;
	QueueIndex present;
	QueueIndex transfer;
	QueueIndex compute;
};

// Device.

constexpr u32 MAX_PHYS_DEVS        = 10u;
constexpr u32 MAX_FORMATS          = 128u;
constexpr u32 MAX_PRESENT_MODES    = 8u;
constexpr u32 MAX_DEVICE_REQ_EXT   = 10u;
constexpr u32 MAX_DEVICE_AVAIL_EXT = 128u;
constexpr u32 MAX_QUEUE_FAMILIES   = 16u;
constexpr u32 MAX_QUEUE_CNT        = 4u;
constexpr u32 INVALID_QUEUE_IDX    = 255u;

struct VulkanDevice
{
	SwapchainSupportInfo swapchain_supp_info;
	VkPhysicalDevice phys_dev;
	VkDevice log_dev;
	VkPhysicalDeviceMemoryProperties mem_props;
	VkQueue graphics_queue;
	VkQueue present_queue;
	VkQueue transfer_queue;
	VkQueue compute_queue;
	QueueIndices queue_indices;
	VkFormat depth_fmt;

	bool init(VulkanContext* ctx);
	bool detect_depth_format();
	Maybe<u32> find_mem_type_index(u32 type_filter, VkMemoryPropertyFlagBits mem_flags);
	void destroy(VulkanContext* ctx);

	void wait_idle() { vkDeviceWaitIdle(this->log_dev); }
	bool graphics_matches_transfer_queue()
	{
		return this->queue_indices.graphics.fam == this->queue_indices.transfer.fam;
	}
};

// Vulkan Context.

struct VulkanContext
{
	VulkanSwapchain swapchain;
	VulkanDevice dev;
    VkInstance instance;
    VkSurfaceKHR surface;
	// TODO: make vk allocator
	VkAllocationCallbacks* vk_alloc;
	sz curr_frame;
	sz curr_shader;
#ifdef RG_DEBUG
	VkDebugUtilsMessengerEXT debug_logger;
#endif

	// FrameData[engine::FRAMES_IN_FLIGHT] frame_data;
	// FixedList{VulkanShader, MAX_SHADER_COUNT} shaders;
	// // 1 pool == 1 cmd_buffer for 1 frame.
	// VulkanCmdPool[env::THREAD_COUNT] graphics_cmd_pools;
	// VulkanCmdPool[env::THREAD_COUNT] transfer_cmd_pools;

	bool init();
};

#define VK_CHECK(res) if ((res) != VK_SUCCESS) panic("VK_CHECK failed, result was %d", res)

} // rg

#endif // _RG_VULKAN_CORE_HPP_
