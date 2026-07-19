#ifndef _RG_ENTRY_HPP_
#define _RG_ENTRY_HPP_

#include "core/basic.hpp"
#include "engine/event.hpp"
#include "engine/input.hpp"
#include "engine/vk_core.hpp"
#include "engine/renderer.hpp"

namespace rg
{

constexpr sz FRAMES_IN_FLIGHT = 2;
constexpr VkClearDepthStencilValue CLEAR_DEPTH_VALUE = { .depth = 1.0f, .stencil = 0 };
constexpr VkOffset2D OFFSET_START = { 0, 0 };

const StrView SHADERS_PATH                           = { CSTR_SIZED("shaders/build") };
const StrView ASSETS_PATH                            = { CSTR_SIZED("assets/") };
const StrView TEXTURES_PATH                          = { CSTR_SIZED("assets/textures/") };
const StrView MODELS_PATH                            = { CSTR_SIZED("assets/models/") };

const StrView VERTEX_SHADER_ENTRY_NAME              = { CSTR_SIZED("vertMain") };
const StrView FRAGMENT_SHADER_ENTRY_NAME            = { CSTR_SIZED("fragMain") };
const StrView COMPUTE_SHADER_ENTRY_NAME             = { CSTR_SIZED("compMain") };
const StrView GEOMETRY_SHADER_ENTRY_NAME            = { CSTR_SIZED("geoMain") };
const StrView TESSELATION_CONTROL_SHADER_ENTRY_NAME = { CSTR_SIZED("tesControlMain") };
const StrView TESSELATION_EVAL_SHADER_ENTRY_NAME    = { CSTR_SIZED("tesEvalMain") };

struct AppConfig
{
    CString window_name;
    u16 window_width;
    u16 window_height;
};

bool application_init(AppConfig config);
void application_run();
void application_destroy();

// Vulkan Context.

struct FrameData
{
	// 1 pool == 1 cmd_buffer for 1 frame.
	Array<VulkanCmdBuffer, RG_THREAD_COUNT> graphics_cmd_buffs;
	Array<VulkanCmdBuffer, RG_THREAD_COUNT> transfer_cmd_buffs;
	Array<CmdBufferState, RG_THREAD_COUNT> graphics_cmd_buff_states;
	Array<CmdBufferState, RG_THREAD_COUNT> transfer_cmd_buff_states;
	VulkanSemaphore image_avail_sem;
	VulkanSemaphore transfer_end_sem;
	VulkanSemaphore render_end_sem;
	VulkanFence render_end_fence;
};

struct VulkanContext
{
	static constexpr sz MAX_SHADER_COUNT = 10;

	VulkanDevice dev;
	VulkanSwapchain swapchain;
    VkSurfaceKHR surface;
	Array<FrameData, FRAMES_IN_FLIGHT> frame_data;
	FArray<VulkanShader, MAX_SHADER_COUNT> shaders;
	Array<VulkanCmdPool, RG_THREAD_COUNT> graphics_cmd_pools;
	Array<VulkanCmdPool, RG_THREAD_COUNT> transfer_cmd_pools;
	sz curr_frame;
	sz curr_shader;
	VkAllocationCallbacks* vk_alloc;
    VkInstance instance;
#ifdef RG_DEBUG
	VkDebugUtilsMessengerEXT debug_logger;
#endif

    bool init(AppConfig config);
    void destroy();

    VulkanShader* get_curr_shader()
    {
    	return &this->shaders[this->curr_shader];
    }
    FrameData* get_curr_frame_data()
    {
    	return &this->frame_data[this->curr_frame];
    }
    void advance_frame()
    {
    	this->curr_frame = this->curr_frame >= (FRAMES_IN_FLIGHT - 1) ? 0 : this->curr_frame + 1;
    }
};

// Window.

struct Window
{
    GLFWwindow* handle;
    u16 width;
    u16 height;

    bool init(AppConfig config);
    void create_vk_surface(VulkanContext* vk_ctx, VkSurfaceKHR* surface);
    VkExtent2D get_screen_coordinates();
    void destroy();
};

// Engine context.

struct EngineContext
{
    EventSystem event_sys;
    InputSystem input_sys;
    VulkanContext vk_ctx;
    Renderer renderer;
    Window window;
};

EngineContext* get_engine_context();

} // rg

#endif // _RG_ENTRY_HPP_
