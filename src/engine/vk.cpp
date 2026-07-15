#include <volk/volk.h>
#include <vulkan/vulkan_core.h>
#include "core/basic.hpp"
#include "collections/farray.hpp"
#include "vk_core.hpp"

namespace rg
{

intern constexpr sz MAX_REQ_LAYERS = 10;
intern constexpr sz MAX_AVAIL_LAYERS = 64;
intern constexpr sz MAX_REQ_EXTENSIONS = 10;
intern constexpr sz MAX_AVAIL_EXTENSIONS = 164;

intern bool add_validation_layers(FArray<CString, MAX_REQ_LAYERS>* out_layers);
intern bool check_req_extensions(FArray<CString, MAX_REQ_EXTENSIONS>* req_extensions);
intern CString get_surface_ext();
intern void init_debug_logger(VulkanContext* ctx);
VkBool32 vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT	message_severity,
	VkDebugUtilsMessageTypeFlagsEXT       	message_types,
	const VkDebugUtilsMessengerCallbackDataEXT*	callback_data,
	void*                                 	user_data
);

bool init_instance(VulkanContext* vk_ctx)
{
	vk_ctx->vk_alloc = null;

    VkApplicationInfo vk_app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_4,
    };

    VkInstanceCreateInfo instance_ci = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &vk_app_info,
    };

#ifdef RG_DEBUG
    FArray<CString, MAX_REQ_LAYERS> req_layers;
	req_layers.push("VK_LAYER_KHRONOS_validation");

	if (!add_validation_layers(&req_layers))
	{
		return false;
	}
#endif // RG_DEBUG

    FArray<CString, MAX_REQ_EXTENSIONS> req_extensions;
    req_extensions.push(VK_KHR_SURFACE_EXTENSION_NAME);
    req_extensions.push(get_surface_ext());

#ifdef RG_DEBUG
	req_extensions.push(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	if (!check_req_extensions(&req_extensions)) {
		return false;
	}
#endif // RG_DEBUG
	instance_ci.enabledExtensionCount = (uint)req_extensions.count;
	instance_ci.ppEnabledExtensionNames = (CString*)req_extensions.data;

	VK_CHECK(vkCreateInstance(&instance_ci, null, &vk_ctx->instance));

	volkLoadInstanceOnly(vk_ctx->instance);

#ifdef RG_DEBUG
	init_debug_logger(vk_ctx);
#endif
	
    return true;
}

intern bool add_validation_layers(FArray<CString, MAX_REQ_LAYERS>* req_layers)
{
    FArray<VkLayerProperties, MAX_REQ_LAYERS> avail_layers;
	u32 avail_layer_cnt;

	VK_CHECK(vkEnumerateInstanceLayerProperties(&avail_layer_cnt, null));
	avail_layer_cnt = rg::min(MAX_AVAIL_LAYERS, (sz)avail_layer_cnt);
	avail_layers.resize(avail_layer_cnt);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&avail_layer_cnt, avail_layers.data));

	for (CString req_layer : *req_layers)
	{
		for (const auto& avail_layer : avail_layers.slice())
		{
			if (mem_compare_nullterm(avail_layer.layerName, req_layer))
			{
				LOG_INFO("Required layer was found: %s", req_layer);
				goto _OUTER;
			}
		}

		LOG_FATAL("Required layer wasn't found: %s\nexiting", req_layer);
		return false;

		_OUTER:;
	}

	return true;
}

bool check_req_extensions(FArray<CString, MAX_REQ_EXTENSIONS>* req_extensions)
{
    FArray<VkExtensionProperties, MAX_AVAIL_EXTENSIONS> avail_extensions;

	u32 avail_ext_cnt;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(null, &avail_ext_cnt, null));
	avail_ext_cnt = rg::min(MAX_AVAIL_EXTENSIONS, (sz)avail_ext_cnt);
	avail_extensions.resize(avail_ext_cnt);

	VK_CHECK(vkEnumerateInstanceExtensionProperties(null, &avail_ext_cnt, avail_extensions.data));

	for (CString req_ext : *req_extensions)
	{
		for (const auto& avail_ext : avail_extensions)
		{
			if (mem_compare_nullterm(avail_ext.extensionName, req_ext)) goto _OUTER;
		}

		LOG_FATAL("Required extension wasn't found: %s\nexiting", req_ext);
		return false;

		_OUTER:;
	}

	return true;
}

void init_debug_logger(VulkanContext* vk_ctx)
{
    u32 log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = log_severity,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .pfnUserCallback = vk_debug_callback,
    };

	PFN_vkCreateDebugUtilsMessengerEXT create_debug_messenger =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_ctx->instance, "vkCreateDebugUtilsMessengerEXT");

	ASSERT_MSG(create_debug_messenger != null, "Debug utils extension was not found");

	VK_CHECK(create_debug_messenger(vk_ctx->instance, &debug_create_info, null, &vk_ctx->debug_logger));
}

VkBool32 vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT	message_severity,
	VkDebugUtilsMessageTypeFlagsEXT       	message_types,
	const VkDebugUtilsMessengerCallbackDataEXT*	callback_data,
	void*                                 	user_data
)
{
	switch (message_severity)
	{
	    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
	        LOG_ERROR("%s", callback_data->pMessage);
	        break;
	    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
	        LOG_WARN("%s", callback_data->pMessage);
	        break;
	    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
	        LOG_INFO("%s", callback_data->pMessage);
	        break;
		default: return VK_FALSE;
	}
	return VK_FALSE;
}

#ifdef RG_PLATFORM_WIN32
CString get_surface_ext()
{
	return "VK_KHR_win32_surface";
}
#else
CString get_surface_ext()
{
	return "VK_KHR_xcb_surface";
}
#endif

} // rg
