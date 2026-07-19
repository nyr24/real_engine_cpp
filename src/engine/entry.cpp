#include <stdarg.h>
#include "volk/volk.h"
#include "engine/vk_core.hpp"
#include "core/context.hpp"
#include "engine/entry.hpp"

namespace rg
{

intern Context context;
intern thread_local Arena* temp_allocator;
intern EngineContext engine_context;
intern VmemAllocator* persistent_allocator;

intern void context_init(Allocator* allocator);
intern void context_destroy();
intern bool engine_context_init(AppConfig config);
intern void engine_context_destroy();
intern void init_main_shader(VulkanContext* ctx);
intern void init_frame_data(VulkanContext* ctx);
intern void destroy_frame_data(VulkanContext* ctx);

// Context.

void context_init(Allocator* allocator)
{
    context.allocator = allocator;
    context.rng.init();
    context.logger_mutex.init();
}

void context_destroy()
{
    context.logger_mutex.destroy();
}

Context* get_context()
{
    return &context;
}

void init_temp_allocator(Allocator* backing_alloc, sz capacity)
{
    temp_allocator = Arena::create(backing_alloc, capacity);
}

Arena* get_temp_allocator()
{
    ASSERT_NON_NULL(temp_allocator);
    return temp_allocator;
}

Allocator* get_persist_allocator()
{
    return context.allocator;
}

// Application callbacks.

bool application_init(AppConfig config)
{
    persistent_allocator = VmemAllocator::create(2 * GB);
    context_init(persistent_allocator);
    init_temp_allocator(context.allocator);
    if (!engine_context_init(config)) return false;
    return true;
}

void application_run()
{
    // TODO("make app run!");
}

void application_destroy()
{
    context_destroy();
    persistent_allocator->destroy();
}

// Engine context.

bool engine_context_init(AppConfig config)
{
    EngineContext* engine_context = get_engine_context();

    if (!engine_context->window.init(config))
        return false;
    if (!engine_context->vk_ctx.init(config))
        return false;

    engine_context->event_sys.init();
    engine_context->input_sys.init();

    return true;
}

void engine_context_destroy()
{
    EngineContext* engine_context = get_engine_context();
    engine_context->vk_ctx.destroy();
}

EngineContext* get_engine_context()
{
    return &engine_context; 
}

// Vulkan initialization.

bool VulkanContext::init(AppConfig config)
{
	VK_CHECK(volkInitialize());

	EngineContext* engine_ctx = get_engine_context();
	VulkanContext* vk_ctx = &engine_ctx->vk_ctx;

	if (!init_instance(vk_ctx)) return false;

	engine_ctx->window.create_vk_surface(vk_ctx, &vk_ctx->surface);

	if (!vk_ctx->dev.init(vk_ctx)) return false;

	init_frame_data(vk_ctx);

	VkExtent2D extent = { (u32)config.window_width, (u32)config.window_height };
	if (!vk_ctx->swapchain.init(vk_ctx, extent)) return false;

	init_main_shader(vk_ctx);
	return true;
}

intern void init_main_shader(VulkanContext* ctx)
{
	constexpr sz MAIN_SHADER_MAX_ENTITY_COUNT         = 1024;
	constexpr CString MAIN_SHADER_NAME                = "shader_default.spv";
	constexpr sz MAIN_SHADER_BINDING_COUNT_PER_ENTITY = 1;
	constexpr sz MAIN_SHADER_SET_COUNT_PER_ENTITY     = 1;

	Array<Array<VkDescriptorType, MAIN_SHADER_BINDING_COUNT_PER_ENTITY>, MAIN_SHADER_SET_COUNT_PER_ENTITY> types_per_set = {
		{
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			// vk::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			// vk::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			// vk::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		}
	};
	Array<Array<VkDescriptorSetLayoutBinding, MAIN_SHADER_BINDING_COUNT_PER_ENTITY>, MAIN_SHADER_SET_COUNT_PER_ENTITY> bindings_per_set;
	Array<VkShaderStageFlags, MAIN_SHADER_SET_COUNT_PER_ENTITY> stages = { VK_SHADER_STAGE_FRAGMENT_BIT };
	Array<DescriptorSetInfo, MAIN_SHADER_SET_COUNT_PER_ENTITY> set_infos;

	for (u32 i = 0; i < set_infos.len(); ++i)
	{
	    DescriptorSetInfo* set_info = &set_infos[i];
		init_descriptor_bindings(stages[i], types_per_set[i].slice(), bindings_per_set[i].slice());
    	set_info->bindings = bindings_per_set[i].slice();
    	set_info->types = types_per_set[i].slice();
	}

	// Attribute descriptions.

	Array<VkVertexInputAttributeDescription, 3> attrib_descriptions = {
	    // Position.
		{ .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0, },
		// Normal.
		{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = sizeof(f32) * 3,
		},
		// Texture coordinate.
		{ .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = sizeof(f32) * 3 * 2, },
	};

	Array<VkPushConstantRange, 1> push_constant_ranges = {
		{ .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(f32) * 16, }
	};

	Array<VulkanPipelineConfig, 1> pipeline_configs = {
		{
			.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.polygon_mode = VK_POLYGON_MODE_FILL,
			.line_width = 1.0f,
			.sample_count = VK_SAMPLE_COUNT_1_BIT,
			.enable_color_blend = true,
		}
	};

	VulkanShaderConfig shader_conf = {
		.descriptor_set_infos = set_infos.slice(),
		.attrib_descriptions = attrib_descriptions.slice(),
		.pipeline_configs = pipeline_configs.slice(),
		.push_constant_ranges = push_constant_ranges.slice(),
		.max_entities_per_frame = MAIN_SHADER_MAX_ENTITY_COUNT,
		.vertex_binding_index = 0,
	};

	shader_conf.stage_bits.set((u8)ShaderStageKind::VERTEX);
	shader_conf.stage_bits.set((u8)ShaderStageKind::FRAGMENT);

	ctx->curr_shader = 0;
	ctx->shaders.push(create_shader(ctx, MAIN_SHADER_NAME, &shader_conf));
}

intern void init_frame_data(VulkanContext* ctx)
{
	if (!ctx->dev.graphics_matches_transfer_queue())
	{
		LOG_INFO("Graphics and Transfer queues are different");

		for (sz i = 0; i < ctx->graphics_cmd_pools.len(); ++i)
		{
		    VulkanCmdPool* graphics_pool = &ctx->graphics_cmd_pools[i];
			graphics_pool->init(ctx, ctx->dev.queue_indices.graphics.fam);

			Array<VulkanCmdBuffer, FRAMES_IN_FLIGHT> graphics_buffs;
			graphics_pool->allocate_cmd_buffers(&ctx->dev, graphics_buffs.slice());

			for (sz j = 0; j < ctx->frame_data.len(); ++j)
			{
			    FrameData* frame = &ctx->frame_data[j];
				frame->graphics_cmd_buffs[i] = graphics_buffs[j];
			}
		}

		for (sz i = 0; i < ctx->transfer_cmd_pools.len(); ++i)
		{
		    VulkanCmdPool* transfer_pool = &ctx->transfer_cmd_pools[i];
			transfer_pool->init(ctx, ctx->dev.queue_indices.transfer.fam);
			Array<VulkanCmdBuffer, FRAMES_IN_FLIGHT> transfer_buffs;
			transfer_pool->allocate_cmd_buffers(&ctx->dev, transfer_buffs.slice());

			for (sz j = 0; j < ctx->frame_data.len(); ++j)
			{
			    FrameData* frame = &ctx->frame_data[j];
				frame->transfer_cmd_buffs[i] = transfer_buffs[j];
			}
		}

		for (FrameData& frame : ctx->frame_data)
		{
			for (CmdBufferState& state : frame.graphics_cmd_buff_states)
			{
				state = CmdBufferState::READY;
			}
			for (CmdBufferState& state : frame.transfer_cmd_buff_states)
			{
				state = CmdBufferState::READY;
			}
		}
	}
	else
	{
		LOG_INFO("Graphics and Transfer queues are the same");

		for (sz i = 0; i < ctx->graphics_cmd_pools.len(); ++i)
		{
		    VulkanCmdPool* graphics_pool = &ctx->graphics_cmd_pools[i];
			graphics_pool->init(ctx, ctx->dev.queue_indices.graphics.fam);
			// Allocate twise.
			Array<VulkanCmdBuffer, FRAMES_IN_FLIGHT * 2> cmd_buffs;
			graphics_pool->allocate_cmd_buffers(&ctx->dev, cmd_buffs.slice());

			sz j = 0;
			for (; j < ctx->frame_data.len(); ++j)
			{
			    FrameData* frame = &ctx->frame_data[j];
				frame->graphics_cmd_buffs[i] = cmd_buffs[j++];
				frame->transfer_cmd_buffs[i] = cmd_buffs[j++];
			}
		}
	}

	for (FrameData& frame : ctx->frame_data)
	{
    	for (CmdBufferState& state : frame.graphics_cmd_buff_states)
		{
			state = CmdBufferState::READY;
		}
    	for (CmdBufferState& state : frame.transfer_cmd_buff_states)
		{
			state = CmdBufferState::READY;
		}
		frame.image_avail_sem.init(ctx);
		frame.transfer_end_sem.init(ctx);
		frame.render_end_sem.init(ctx);
		frame.render_end_fence.init(ctx);
	}
}

intern void destroy_frame_data(VulkanContext* ctx)
{
	if (!ctx->dev.graphics_matches_transfer_queue())
	{
		for (sz i = 0; i < ctx->graphics_cmd_pools.len(); ++i)
		{
		    VulkanCmdPool* graphics_pool = &ctx->graphics_cmd_pools[i];
			Array<VulkanCmdBuffer, FRAMES_IN_FLIGHT> graphics_buffs;

			for (sz j = 0; j < ctx->frame_data.len(); ++j)
			{
			    FrameData* frame = &ctx->frame_data[j];
				graphics_buffs[j] = frame->graphics_cmd_buffs[i];
			}

			graphics_pool->destroy_cmd_buffers(&ctx->dev, graphics_buffs.slice());
			graphics_pool->destroy(ctx);
		}

		for (sz i = 0; i < ctx->transfer_cmd_pools.len(); ++i)
		{
		    VulkanCmdPool* transfer_pool = &ctx->transfer_cmd_pools[i];
			Array<VulkanCmdBuffer, FRAMES_IN_FLIGHT> transfer_buffs;

			for (sz j = 0; j < ctx->frame_data.len(); ++j)
			{
			    FrameData* frame = &ctx->frame_data[j];
				transfer_buffs[j] = frame->transfer_cmd_buffs[i];
			}

			transfer_pool->destroy_cmd_buffers(&ctx->dev, transfer_buffs.slice());
			transfer_pool->destroy(ctx);
		}
	}
	else
	{
		for (sz i = 0; i < ctx->graphics_cmd_pools.len(); ++i)
		{
		    VulkanCmdPool* graphics_pool = &ctx->graphics_cmd_pools[i];
			Array<VulkanCmdBuffer, FRAMES_IN_FLIGHT * 2> cmd_buffs;

			sz j = 0;
			for (FrameData& frame : ctx->frame_data)
			{
				cmd_buffs[j++] = frame.graphics_cmd_buffs[i];
				cmd_buffs[j++] = frame.transfer_cmd_buffs[i];
			}

			graphics_pool->destroy_cmd_buffers(&ctx->dev, cmd_buffs.slice());
			graphics_pool->destroy(ctx);
			// Nullify already freed transfer pool.
			ctx->transfer_cmd_pools[i] = { };
		}
	}

	for (FrameData& frame : ctx->frame_data)
	{
		// Assign 'NOT_ALLOCATED' (0) state to cmd buffer states.
		frame.graphics_cmd_buff_states = { };
		frame.transfer_cmd_buff_states = { };

		frame.image_avail_sem.destroy(ctx);
		frame.render_end_sem.destroy(ctx);
		frame.transfer_end_sem.destroy(ctx);
		frame.render_end_fence.destroy(ctx);
	}
}

void VulkanContext::destroy()
{
	for (VulkanShader& shader : this->shaders)
	{
		shader.destroy(this);
	}
	destroy_frame_data(this);
	this->swapchain.destroy(this);
	this->dev.destroy(this);
	vkDestroyInstance(this->instance, this->vk_alloc);
}

// Logging.

constexpr EnumArray<CString, LogLevel> LOG_COLORS = {
    "\x1b[1;32m",
    "\x1b[1;34m",
    "\x1b[1;34m",
    "\x1b[45;37m",
    "\x1b[1;33m",
    "\x1b[1;31m",
    "\x1b[0;41m",
};

constexpr EnumArray<CString, LogLevel> LOG_INTROS = {
    "\x1b[1;32m[INFO]: ",
    "\x1b[1;34m[TRACE]: ",
    "\x1b[1;34m[DEBUG]: ",
    "\x1b[45;37m[TEST]: ",
    "\x1b[1;33m[WARN]: ",
    "\x1b[1;31m[ERROR]: ",
    "\x1b[0;41m[FATAL]: ",
};

const CString LOG_COLOR_RESET = "\x1b[0m\n";

void log_proc(LogLevel level, CString fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Context* ctx = get_context();
    ctx->logger_mutex.lock();
    printn(LOG_INTROS[level]);
    vfprintf(stdout, fmt, args);
    printn(LOG_COLOR_RESET);
    ctx->logger_mutex.unlock();
    va_end(args);
}

void log_proc_scoped(CString fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Context* ctx = get_context();
    ctx->logger_mutex.lock();
    vfprintf(stdout, fmt, args);
    ctx->logger_mutex.unlock();
    va_end(args);
}

void set_log_scope(LogLevel level, CString msg)
{
    printfn("%s %s", LOG_INTROS[level], msg);
}

void set_log_scope(LogLevel level)
{
    printn(LOG_COLORS[level]);
}

void reset_log_scope()
{
    printn(LOG_COLOR_RESET);
}

} // rg
