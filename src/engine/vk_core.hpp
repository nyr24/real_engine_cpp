#ifndef _RG_VULKAN_CORE_HPP_
#define _RG_VULKAN_CORE_HPP_

#include <vulkan/vulkan.h>

namespace rg
{

struct VulkanContext
{
    VkInstance instance;
    VkSurfaceKHR surface;
	VkAllocationCallbacks* vk_alloc;
#ifdef RG_DEBUG
	VkDebugUtilsMessengerEXT debug_logger;
#endif

	// FrameData[engine::FRAMES_IN_FLIGHT] frame_data;
	// VulkanDevice dev;
	// VulkanSwapchain swapchain;
	// VulkanRenderer renderer;
	// FixedList{VulkanShader, MAX_SHADER_COUNT} shaders;
	// // TODO: make vk allocator
	// sz curr_frame;
	// sz curr_shader;
	// // 1 pool == 1 cmd_buffer for 1 frame.
	// VulkanCmdPool[env::THREAD_COUNT] graphics_cmd_pools;
	// VulkanCmdPool[env::THREAD_COUNT] transfer_cmd_pools;
	// VkInstance instance;
	// VkSurfaceKHR surface;
	// App* app_ctx;
};

bool vk_context_init(VulkanContext* ctx);

#define VK_CHECK(res) if ((res) != VK_SUCCESS) panic("VK_CHECK failed, result was %d", res)

} // rg

#endif // _RG_VULKAN_CORE_HPP_
