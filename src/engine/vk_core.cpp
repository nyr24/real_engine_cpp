#include "volk/volk.h"
#include "core/basic.hpp"
#include "collections/farray.hpp"
#include "collections/darray.hpp"
#include "collections/string.hpp"
#include "core/context.hpp"
#include "core/io.hpp"
#include "engine/vk_core.hpp"
#include "engine/entry.hpp"
#include "engine/geometry.hpp"
#include "engine/entity.hpp"

namespace rg
{

intern constexpr sz MAX_REQ_LAYERS = 10;
intern constexpr sz MAX_AVAIL_LAYERS = 64;
intern constexpr sz MAX_REQ_EXTENSIONS = 10;
intern constexpr sz MAX_AVAIL_EXTENSIONS = 164;

intern bool add_validation_layers(DArray<CString>* out_layers);
intern bool check_req_extensions(DArray<CString>* req_extensions);
intern void init_debug_logger(VulkanContext* ctx);
VkBool32 vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT	message_severity,
	VkDebugUtilsMessageTypeFlagsEXT       	message_types,
	const VkDebugUtilsMessengerCallbackDataEXT*	callback_data,
	void*                                 	user_data
);

// Instance.

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

    Arena* talloc = get_temp_allocator();
    TEMP_ALLOC_SCOPE(talloc);

#ifdef RG_DEBUG
    DArray<CString> req_layers;
    req_layers.init_capacity(talloc, MAX_REQ_LAYERS);
	req_layers.push("VK_LAYER_KHRONOS_validation");

	if (!add_validation_layers(&req_layers))
	{
		return false;
	}
#endif // RG_DEBUG

    DArray<CString> req_extensions;
    req_extensions.init_capacity(talloc, MAX_REQ_EXTENSIONS);

    u32 platform_extensions_count;
	CString* platform_extensions = glfwGetRequiredInstanceExtensions(&platform_extensions_count);
	for (sz i = 0; i < platform_extensions_count; ++i)
	{
		req_extensions.push(platform_extensions[i]);
	}

#ifdef RG_DEBUG
	req_extensions.push(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	if (!check_req_extensions(&req_extensions)) {
		return false;
	}
#endif // RG_DEBUG
	instance_ci.enabledExtensionCount = (u32)req_extensions.count;
	instance_ci.ppEnabledExtensionNames = (CString*)req_extensions.data;

	VK_CHECK(vkCreateInstance(&instance_ci, vk_ctx->vk_alloc, &vk_ctx->instance));

	volkLoadInstanceOnly(vk_ctx->instance);

#ifdef RG_DEBUG
	init_debug_logger(vk_ctx);
#endif
	
    return true;
}

intern bool add_validation_layers(DArray<CString>* req_layers)
{
    DArray<VkLayerProperties> avail_layers;
    avail_layers.tinit_capacity(MAX_AVAIL_LAYERS);
	u32 avail_layer_count;

	VK_CHECK(vkEnumerateInstanceLayerProperties(&avail_layer_count, null));
	avail_layer_count = rg::min(MAX_AVAIL_LAYERS, (sz)avail_layer_count);
	avail_layers.resize(avail_layer_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&avail_layer_count, avail_layers.data));

	for (CString req_layer : *req_layers)
	{
		for (const auto& avail_layer : avail_layers)
		{
			if (rg::mem_compare_nullterm(avail_layer.layerName, req_layer))
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

intern bool check_req_extensions(DArray<CString>* req_extensions)
{
    DArray<VkExtensionProperties> avail_extensions;
    avail_extensions.tinit_capacity(MAX_AVAIL_EXTENSIONS);

	u32 avail_ext_count;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(null, &avail_ext_count, null));
	avail_ext_count = rg::min(MAX_AVAIL_EXTENSIONS, (sz)avail_ext_count);
	avail_extensions.resize(avail_ext_count);

	VK_CHECK(vkEnumerateInstanceExtensionProperties(null, &avail_ext_count, avail_extensions.data));

	for (CString req_ext : *req_extensions)
	{
		for (const auto& avail_ext : avail_extensions)
		{
			if (rg::mem_compare_nullterm(avail_ext.extensionName, req_ext)) goto _OUTER;
		}

		LOG_FATAL("Required extension wasn't found: %s\nexiting", req_ext);
		return false;

		_OUTER:;
	}

	return true;
}

intern void init_debug_logger(VulkanContext* vk_ctx)
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

// Device.

intern bool is_phys_device_suitable(
	VkPhysicalDevice dev,
	VkSurfaceKHR surface,
	Slice<CString> req_extensions,
	SwapchainSupportInfo* out_swapchain_supp
);

intern bool find_queue_families(
	VkPhysicalDevice dev,
	VkSurfaceKHR surface,
	DArray<u8>* selected_fam_queue_counts,
	QueueIndices* out_indices
);

intern void calc_required_queue_counts(
	Slice<u8> selected_queue_counts,
	DArray<QueueCount>* out_queue_counts
);

intern void query_swapchain_support(
	VkPhysicalDevice dev,
	VkSurfaceKHR surface,
	SwapchainSupportInfo* out_swapchain_supp
);

intern Maybe<QueueIndex> pick_queue(
	Slice<VkQueueFamilyProperties> queue_families,
	Slice<u8> selected_fam_queue_counts,
	VkQueueFlagBits required_flag,
	VkQueueFlagBits optional_flags,
	VkQueueFlagBits not_needed_flags,
	Slice<u8> not_needed_picked_queue_indices,
	u8 min_queue_count = u8(1)
);

intern Maybe<QueueIndex> pick_presentation_queue(
	Slice<VkQueueFamilyProperties> queue_families,
	Slice<u8> selected_fam_queue_counts,
	VkPhysicalDevice dev,
	VkSurfaceKHR surface,
	// if user specifically wants to combine presentation queue with graphics
	// it can result in performance benefits,
	// because you don't need to synch swapchain images between separate queues.
	Maybe<QueueIndex> mb_graphics_queue_combine_index,
	Slice<u8> not_needed_indices
);

// Device impl.

bool VulkanDevice::init(VulkanContext* ctx)
{
	VulkanDevice* dev = &ctx->dev;
	u32 device_count;
	VK_CHECK(vkEnumeratePhysicalDevices(ctx->instance, &device_count, null));
	if (!device_count)
	{
		LOG_FATAL("Available devices were not found");
		return false;
	}

	Arena* talloc = get_temp_allocator();
	TEMP_ALLOC_SCOPE(talloc);

	DArray<VkPhysicalDevice> phys_devs;
	phys_devs.init_capacity(talloc, device_count);
	phys_devs.resize(device_count);

	VK_CHECK(vkEnumeratePhysicalDevices(ctx->instance, &device_count, phys_devs.data));

	FArray<CString, MAX_DEVICE_REQ_EXT> device_req_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
	};

	VkPhysicalDevice selected_phys_dev = null;
	Slice<CString> req_ext_slice = device_req_extensions.slice();

	for (const auto& phys_dev : phys_devs.slice())
	{
		if (is_phys_device_suitable(phys_dev, ctx->surface, req_ext_slice, &ctx->dev.swapchain_supp_info))
		{
			selected_phys_dev = phys_dev;
			break;
		}
	}

	if (!selected_phys_dev)
	{
		LOG_FATAL("Appropriate device was not found");
		return false;
	}

	dev->phys_dev = selected_phys_dev;

	if (!dev->detect_depth_format())
	{
		LOG_FATAL("Suitable depth format was not detected");
		return false;
	}

	vkGetPhysicalDeviceMemoryProperties(selected_phys_dev, &dev->mem_props);

	// Queue Selection.

	// Here we're tracking how much queues were selected from each family.
	DArray<u8> selected_queue_counts_by_family;
	selected_queue_counts_by_family.init(talloc);

	if (!find_queue_families(selected_phys_dev, ctx->surface, &selected_queue_counts_by_family, &dev->queue_indices))
	{
		LOG_FATAL("Failed to find required queue families on the device");
		return false;
	}

	DArray<QueueCount> compact_queue_fam_counts;
	compact_queue_fam_counts.tinit_capacity(MAX_QUEUE_FAMILIES);
	calc_required_queue_counts(selected_queue_counts_by_family.slice(), &compact_queue_fam_counts);

	FArray<VkDeviceQueueCreateInfo, MAX_QUEUE_FAMILIES> queue_infos;
	queue_infos.resize(compact_queue_fam_counts.count);

	FArray<f32, MAX_QUEUE_FAMILIES> queue_priorities;
	queue_priorities.resize(compact_queue_fam_counts.count);
	queue_priorities.fill(1.0f);

	for (sz i = 0; i < queue_infos.count; ++i)
	{
		VkDeviceQueueCreateInfo* info = &queue_infos[i];
		if (compact_queue_fam_counts[i].count == 0)
		{
			continue;
		}

		info->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info->pNext = null;
		info->queueFamilyIndex = compact_queue_fam_counts[i].fam_ind;
		info->queueCount = compact_queue_fam_counts[i].count;
		info->flags = 0;
		info->pQueuePriorities = &queue_priorities[i];
	}

	VkPhysicalDeviceFeatures device_features = { .samplerAnisotropy = VK_TRUE, };

	VkPhysicalDeviceVulkan13Features device_features13 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.synchronization2 = VK_TRUE,
		.dynamicRendering = VK_TRUE,
	};

	VkPhysicalDeviceVulkan12Features device_features12 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &device_features13,
		.descriptorIndexing = VK_TRUE,
		.bufferDeviceAddress = VK_TRUE,
	};

	VkDeviceCreateInfo device_ci = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &device_features12,
		.queueCreateInfoCount = (u32)queue_infos.count,
		.pQueueCreateInfos = queue_infos.first_ref(),
		.enabledExtensionCount = (u32)device_req_extensions.count,
		.ppEnabledExtensionNames = device_req_extensions.data,
		.pEnabledFeatures = &device_features,
	};

	VK_CHECK(vkCreateDevice(dev->phys_dev, &device_ci, ctx->vk_alloc, &dev->log_dev));
	volkLoadDevice(dev->log_dev);

	vkGetDeviceQueue(
		dev->log_dev,
		dev->queue_indices.graphics.fam,
		dev->queue_indices.graphics.queue,
		&dev->graphics_queue
	);
	vkGetDeviceQueue(dev->log_dev, dev->queue_indices.present.fam, dev->queue_indices.present.queue, &dev->present_queue);
	vkGetDeviceQueue(
		dev->log_dev,
		dev->queue_indices.transfer.fam,
		dev->queue_indices.transfer.queue,
		&dev->transfer_queue
	);
	vkGetDeviceQueue(dev->log_dev, dev->queue_indices.compute.fam, dev->queue_indices.compute.queue, &dev->compute_queue);

	return true;
}

Maybe<u32> VulkanDevice::find_mem_type_index(u32 type_filter, VkMemoryPropertyFlagBits mem_flags)
{
	Maybe<u32> res;
	Slice<VkMemoryType> mem_types_view = { this->mem_props.memoryTypes, this->mem_props.memoryTypeCount };
	for (u32 i = 0; i < (u32)mem_types_view.count; ++i)
	{
		VkMemoryType& mem_type = mem_types_view[i];
		if (((type_filter & (1 << i)) > 0) && ((mem_type.propertyFlags & mem_flags) == mem_flags))
		{
			res.set_val(i);
			return res;
		}
	}

	LOG_FATAL("Failed to find memory type index");
	return res;
}

bool is_phys_device_suitable(
	VkPhysicalDevice dev,
	VkSurfaceKHR surface,
	Slice<CString> req_extensions,
	SwapchainSupportInfo* out_swapchain_supp
)
{
	VkPhysicalDeviceProperties dev_props;
	vkGetPhysicalDeviceProperties(dev, &dev_props);
	if (dev_props.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		LOG_TRACE("Device is not suitable, not a discrete GPU, skipping...");
		return false;
	}

	query_swapchain_support(dev, surface, out_swapchain_supp);
	if (out_swapchain_supp->present_modes.count == 0)
	{
		LOG_TRACE("Physical device is not suitable, doesn't have present modes, skipping...");
		return false;
	}

	Arena* talloc = get_temp_allocator(); 
	TEMP_ALLOC_SCOPE(talloc);
	DArray<VkExtensionProperties> avail_extensions;

	u32 avail_ext_count;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(dev, null, &avail_ext_count, null));
	avail_extensions.init_capacity(talloc, (sz)avail_ext_count);
	avail_extensions.resize((sz)avail_ext_count);

	VK_CHECK(vkEnumerateDeviceExtensionProperties(dev, null, &avail_ext_count, &avail_extensions[0]));

	for (const auto& req_ext : req_extensions)
	{
		bool found = false;
		for (const auto& avail_ext : avail_extensions.slice())
		{
			if (rg::mem_compare_nullterm(avail_ext.extensionName, req_ext))
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			LOG_TRACE("Required device extension was not found, extension name: %s, skipping...", req_ext);
			return false;
		}
	}

	VkPhysicalDeviceFeatures dev_features;
	vkGetPhysicalDeviceFeatures(dev, &dev_features);

	if (!dev_features.samplerAnisotropy)
	{
		LOG_TRACE("Physical device does not have sampler_anisotropy, which is required feature, skipping...");
		return false;
	}

	return true;
}

intern void query_swapchain_support(
	VkPhysicalDevice dev,
	VkSurfaceKHR surface,
	SwapchainSupportInfo* out_swapchain_supp
)
{
	ASSERT_NON_NULL(surface);

	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &out_swapchain_supp->capabilities));

	u32 format_count;
	u32 present_mode_count;
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &format_count, null));
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &present_mode_count, null));

	if (format_count > 0)
	{
		out_swapchain_supp->formats.resize(rg::min((u32)MAX_FORMATS, format_count));
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &format_count, out_swapchain_supp->formats.data));
	}

	if (present_mode_count > 0)
	{
		out_swapchain_supp->present_modes.resize(rg::min((u32)MAX_PRESENT_MODES, present_mode_count));
		VK_CHECK(
			vkGetPhysicalDeviceSurfacePresentModesKHR(
				dev,
				surface,
				&present_mode_count,
				out_swapchain_supp->present_modes.data
			)
		);
	}
}

bool VulkanDevice::detect_depth_format()
{
	FArray<VkFormat, 3> candidates = {
		VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, 		
	};

	u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

	for (const auto& cand : candidates)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(this->phys_dev, cand, &properties);
		if (((properties.linearTilingFeatures & flags) == flags)
			|| ((properties.optimalTilingFeatures & flags) == flags))
		{
			this->depth_fmt = cand;
			return true;
		}
	}

	return false;
}

intern bool find_queue_families(
	VkPhysicalDevice dev,
	VkSurfaceKHR surface,
	DArray<u8>* selected_queue_counts_by_family,
	QueueIndices* out_indices
)
{
	ASSERT_INITIALIZED(selected_queue_counts_by_family);

	Arena* talloc = get_temp_allocator();
	TEMP_ALLOC_SCOPE(talloc);

	u32 queue_fam_count;
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_fam_count, null);

	DArray<VkQueueFamilyProperties> queue_families;
	queue_families.init_capacity(talloc, queue_fam_count);
	queue_families.resize(queue_fam_count);

	selected_queue_counts_by_family->resize(queue_fam_count);
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_fam_count, queue_families.data);

	Slice<VkQueueFamilyProperties> queue_family_view = queue_families.slice();
	Slice<u8> selected_queue_counts_by_family_view = selected_queue_counts_by_family->slice();

	FArray<u8, MAX_QUEUE_FAMILIES> not_needed_fam_indices;

	{
		auto [idx, is_found] = pick_queue(
			queue_family_view,
			selected_queue_counts_by_family_view,
			VK_QUEUE_GRAPHICS_BIT,
			(VkQueueFlagBits)0,
			VK_QUEUE_TRANSFER_BIT,
			{}
		);
		if (is_found)
		{
			out_indices->graphics = idx;
			selected_queue_counts_by_family_view[idx.fam]++;
		}
		else
		{
			LOG_FATAL("Appropriate queue for Graphics was not found on selected vulkan device");
			return false;
		}
	}

	{
		not_needed_fam_indices.push(out_indices->graphics.fam);

		auto [idx, is_found] = pick_queue(
			queue_family_view,
			selected_queue_counts_by_family_view,
			VK_QUEUE_TRANSFER_BIT,
			(VkQueueFlagBits)0,
			VK_QUEUE_GRAPHICS_BIT,
			not_needed_fam_indices.slice()
		);
		if (!is_found)
		{
			out_indices->transfer = out_indices->graphics;
		}
		else
		{
			out_indices->transfer = idx;
			selected_queue_counts_by_family_view[idx.fam]++;
		}
	}

	{
		auto [idx, is_found] = pick_queue(
			queue_family_view,
			selected_queue_counts_by_family_view,
			VK_QUEUE_COMPUTE_BIT,
			VK_QUEUE_TRANSFER_BIT,
			VK_QUEUE_GRAPHICS_BIT,
			not_needed_fam_indices.slice()
		);
		if (!is_found)
		{
			out_indices->compute = out_indices->graphics;
		}
		else
		{
			out_indices->compute = idx;
			selected_queue_counts_by_family_view[idx.fam]++;
		}
	}

	{
		not_needed_fam_indices.clear();
		not_needed_fam_indices.push(out_indices->transfer.fam);

		auto [idx, is_found] = pick_presentation_queue(
			queue_family_view,
			selected_queue_counts_by_family_view,
			dev,
			surface,
			Maybe{ out_indices->graphics },
			not_needed_fam_indices.slice()
		);
		if (!is_found)
		{
			out_indices->present = out_indices->graphics;
		}
		else
		{
			out_indices->present = idx;
			selected_queue_counts_by_family_view[idx.fam]++;
		}
	}

	return true;
}

intern Maybe<QueueIndex> pick_queue(
	Slice<VkQueueFamilyProperties> queue_families,
	Slice<u8> selected_fam_queue_counts,
	VkQueueFlagBits required_flag,
	VkQueueFlagBits optional_flags,
	VkQueueFlagBits not_needed_flags,
	Slice<u8> not_needed_picked_queue_indices,
	u8 min_queue_count
)
{
	sz max_score = 0;
	Maybe<QueueIndex> result_index;
	sz curr_score;

	for (u8 i = 0; i < queue_families.count; ++i)
	{
		const VkQueueFamilyProperties& family = queue_families[i];

		if (((family.queueFlags & required_flag) != required_flag) || (family.queueCount <= selected_fam_queue_counts[i])
			|| (family.queueCount < min_queue_count))
		{
			continue;
		}

		curr_score = 1;

		if (not_needed_picked_queue_indices.count)
		{
			if (!not_needed_picked_queue_indices.has(i))
			{
				curr_score += 1;
			}
		}

		if (optional_flags > 0 && (family.queueFlags & optional_flags) == optional_flags)
		{
			curr_score += 1;
		}

		if ((family.queueFlags & not_needed_flags) == 0)
		{
			curr_score += 1;
		}

		if (curr_score > max_score)
		{
			result_index.set_val({ .fam = i, .queue = selected_fam_queue_counts[i] });
			max_score = curr_score;
		}
	}

	return result_index;
}

intern Maybe<QueueIndex> pick_presentation_queue(
	Slice<VkQueueFamilyProperties> queue_families,
	Slice<u8> selected_fam_queue_counts,
	VkPhysicalDevice dev,
	VkSurfaceKHR surface,
	// if user specifically wants to combine presentation queue with graphics
	// it can result in performance benefits,
	// because you don't need to synch swapchain images between separate queues.
	Maybe<QueueIndex> mb_graphics_queue_combine_index,
	Slice<u8> not_needed_indices
)
{
	sz max_score = 0;
	Maybe<QueueIndex> result_index;
	sz curr_score;

	for (u8 i = 0; i < queue_families.count; ++i)
	{
		const VkQueueFamilyProperties& family = queue_families[i];
		u32 supports_presentation;
		VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &supports_presentation));

		if (!supports_presentation)
		{
			continue;
		}

		if (mb_graphics_queue_combine_index)
		{
			if (mb_graphics_queue_combine_index.val.fam == i)
			{
				result_index.set_val(mb_graphics_queue_combine_index.val);
				return result_index;
			}
		}

		curr_score = 1;

		if ((family.queueFlags & VK_QUEUE_GRAPHICS_BIT) > 0)
		{
			curr_score += 1;
		}

		if (not_needed_indices.count)
		{
			if (!not_needed_indices.has(i))
			{
				curr_score += 1;
			}
		}

		if (curr_score > max_score)
		{
			result_index.set_val({ .fam = i, .queue = selected_fam_queue_counts[i] });
			max_score = curr_score;
		}
	}

	return result_index;
}

intern void calc_required_queue_counts(
	Slice<u8> selected_queue_counts,
	DArray<QueueCount>* out_queue_counts
)
{
	for (u8 i = 0; i < selected_queue_counts.count; ++i)
	{
		u8 count = selected_queue_counts[i];
		if (count == 0) continue;
		out_queue_counts->push({ .fam_ind = i, .count = count });
	}
}

void VulkanDevice::destroy(VulkanContext* ctx)
{
	VulkanDevice* dev = &ctx->dev;
	dev->graphics_queue = null;
	dev->present_queue = null;
	dev->transfer_queue = null;

	if (dev->log_dev)
	{
		vkDestroyDevice(dev->log_dev, ctx->vk_alloc);
		dev->log_dev = null;
	}

	dev->phys_dev = null;

	dev->queue_indices.graphics.fam = INVALID_QUEUE_IDX;
	dev->queue_indices.graphics.queue = INVALID_QUEUE_IDX;
	dev->queue_indices.present.fam = INVALID_QUEUE_IDX;
	dev->queue_indices.present.queue = INVALID_QUEUE_IDX;
	dev->queue_indices.transfer.fam = INVALID_QUEUE_IDX;
	dev->queue_indices.transfer.queue = INVALID_QUEUE_IDX;
	dev->queue_indices.compute.fam = INVALID_QUEUE_IDX;
	dev->queue_indices.compute.queue = INVALID_QUEUE_IDX;
}

// Swapchain.

intern bool swapchain_create_inner(VulkanSwapchain* swapchain, VulkanContext* ctx, VkExtent2D extent);

bool operator==(VkExtent2D a, VkExtent2D b)
{
	return a.width == b.width && a.height == b.height;
}

// Swapchain impl.

bool VulkanSwapchain::init(VulkanContext* ctx, VkExtent2D extent)
{
	VulkanSwapchain* swapchain = &ctx->swapchain;
	swapchain->recreation_scheduled = false;
	if (!swapchain->acquire_format_and_present_mode(&ctx->dev.swapchain_supp_info))
	{
		LOG_FATAL("Failed to acquire appropriate format for swapchain");
		return false;
	}
	if (!swapchain_create_inner(swapchain, ctx, extent))
	{
		LOG_FATAL("Failed create swapchain");
		return false;
	}
	return true;
}

SwapchainPresentResult VulkanSwapchain::acquire_next_image_index(
	VulkanContext* ctx,
	VulkanSemaphore image_available_semaphore,
	u64 timeout_ns
)
{
	u32 image_index;
	VkResult result = VK_NOT_READY;

	do
	{
		result = vkAcquireNextImageKHR(
			ctx->dev.log_dev,
			this->handle,
			timeout_ns,
			image_available_semaphore.handle,
			null,
			&image_index
		);
	} while (result == VK_NOT_READY);

	this->curr_image_index = image_index;
	EngineContext* engine_ctx = get_engine_context();

	switch (result)
	{
		case VK_SUCCESS:
			return SwapchainPresentResult::SUCCESS;
		case VK_SUBOPTIMAL_KHR:
			{
				VkExtent2D new_coords = engine_ctx->window.get_screen_coordinates();
				VkExtent2D curr_coords = engine_ctx->renderer.get_extent();
				if (new_coords != curr_coords)
				{
					LOG_INFO("Swapchain is outdated, scheduling recreation...");
					engine_ctx->renderer.update_viewport_scissors(new_coords);
					this->recreation_scheduled = true;
					return SwapchainPresentResult::NEEDS_RECREATION;
				}
				return SwapchainPresentResult::SUCCESS;
			}
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			LOG_WARN("Swapchain is outdated, can't proceed next frame, scheduling recreation...");
			this->recreation_scheduled = true;
			return SwapchainPresentResult::NEEDS_RECREATION_CANT_PROCEED;
		default:
			panic("Failed to acquire swapchain image, reason: %s");
	}
}

SwapchainPresentResult VulkanSwapchain::present(
	VulkanContext* ctx,
	VkQueue present_queue,
	VulkanSemaphore render_complete_semaphore
)
{
	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores = &render_complete_semaphore.handle,
		.swapchainCount = 1u,
		.pSwapchains = &this->handle,
		.pImageIndices = &this->curr_image_index,
		.pResults = null,
	};

	VkResult result = vkQueuePresentKHR(present_queue, &present_info);
	EngineContext* engine_ctx = get_engine_context();
	
	switch (result)
	{
		case VK_SUCCESS:
			return SwapchainPresentResult::SUCCESS;
		case VK_SUBOPTIMAL_KHR:
			{
				VkExtent2D new_coords = engine_ctx->window.get_screen_coordinates();
				VkExtent2D prev_coords = engine_ctx->renderer.scissors.extent;
				if (new_coords != prev_coords)
				{
					LOG_INFO("Swapchain is outdated, scheduling recreation...");
					this->recreation_scheduled = true;
					return SwapchainPresentResult::NEEDS_RECREATION;
				}
				return SwapchainPresentResult::SUCCESS;
			}
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			LOG_WARN("Swapchain is outdated, can't proceed next frame, scheduling recreation...");
			this->recreation_scheduled = true;
			return SwapchainPresentResult::NEEDS_RECREATION_CANT_PROCEED;
		default:
			panic("Failed to present swapchain image, reason: %d", result);
	}
}

bool VulkanSwapchain::recreate(VulkanContext* ctx, VkExtent2D extent)
{
	ctx->dev.wait_idle();
	this->destroy(ctx);
	return swapchain_create_inner(this, ctx, extent);
}

void VulkanSwapchain::recreate_if_needed(VulkanContext* ctx)
{
	if (this->recreation_scheduled)
	{
		EngineContext* engine_ctx = get_engine_context();
		VkExtent2D new_coords = engine_ctx->renderer.get_extent();
		LOG_INFO("Recreating swapchain, new screen coordinates: %d %d", new_coords.width, new_coords.height);
		this->recreate(ctx, new_coords);
		this->recreation_scheduled = false;
		engine_ctx->renderer.set_resize_scheduled(true);
	}
}

intern bool swapchain_create_inner(VulkanSwapchain* swapchain, VulkanContext* ctx, VkExtent2D extent)
{
	VulkanDevice* dev               = &ctx->dev;
	SwapchainSupportInfo* supp_info = &dev->swapchain_supp_info;

	VkExtent2D min_extent = supp_info->capabilities.minImageExtent;
	VkExtent2D max_extent = supp_info->capabilities.maxImageExtent;
	extent.width = rg::max(extent.width, min_extent.width);
	extent.height = rg::max(extent.height, min_extent.height);

	if (max_extent.width > min_extent.width)
	{
		extent.width = rg::min(extent.width, max_extent.width);
		extent.height = rg::min(extent.height, max_extent.height);
	}

	u32 create_image_count = supp_info->capabilities.minImageCount + 1;
	if (supp_info->capabilities.maxImageCount > 0 && create_image_count > supp_info->capabilities.maxImageCount)
	{
		create_image_count = supp_info->capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchain_ci = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = ctx->surface,
		.minImageCount = create_image_count,
		.imageFormat = swapchain->surface_fmt.format,
		.imageColorSpace = swapchain->surface_fmt.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = supp_info->capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = swapchain->present_mode,
		.oldSwapchain = swapchain->handle,
	};

	Array<u32, 2> queue_indices = { dev->queue_indices.graphics.fam, dev->queue_indices.present.fam };

	if (dev->queue_indices.graphics.fam != dev->queue_indices.present.fam)
	{
		swapchain->sharing_mode = VK_SHARING_MODE_CONCURRENT;
		swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_ci.queueFamilyIndexCount = queue_indices.len();
		swapchain_ci.pQueueFamilyIndices = queue_indices.data;
	}
	else
	{
		swapchain->sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_ci.queueFamilyIndexCount = 0;
		swapchain_ci.pQueueFamilyIndices = null;
	}

	VK_CHECK(vkCreateSwapchainKHR(dev->log_dev, &swapchain_ci, ctx->vk_alloc, &ctx->swapchain.handle));

	u32 actual_image_count;
	VK_CHECK(vkGetSwapchainImagesKHR(dev->log_dev, swapchain->handle, &actual_image_count, null));

	actual_image_count = rg::min((u32)VulkanSwapchain::MAX_SWAPCHAIN_IMAGE_COUNT, actual_image_count);
	if (swapchain->images.count != actual_image_count)
	{
		swapchain->images.resize(actual_image_count);
	}
	if (swapchain->image_views.count != actual_image_count)
	{
		swapchain->image_views.resize(actual_image_count);
	}

	VK_CHECK(vkGetSwapchainImagesKHR(dev->log_dev, swapchain->handle, &actual_image_count, swapchain->images.data));

	for (sz i = 0; i < swapchain->image_views.count; ++i)
	{
		swapchain->image_views[i] = create_view_from_raw_image(ctx, swapchain->images[i], swapchain->surface_fmt.format);
	}

	if (!dev->detect_depth_format())
	{
		dev->depth_fmt = VK_FORMAT_UNDEFINED;
		LOG_FATAL("Failed to find a supported format");
		return false;
	}

	swapchain->depth_image.init(
		ctx,
		dev->depth_fmt,
		extent.width,
		extent.height,
		1u,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	swapchain->curr_image_index = 0;
	LOG_INFO("Swapchain created successfully!");
	LOG_INFO(
		"Image sharing mode: %s",
		swapchain->sharing_mode == VK_SHARING_MODE_CONCURRENT ? "Concurrent" : "Exclusive"
	);
	return true;
}

bool VulkanSwapchain::acquire_format_and_present_mode(SwapchainSupportInfo* supp_info)
{
	bool found = false;
	for (auto& fmt : supp_info->formats)
	{
		if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			found = true;
			this->surface_fmt = fmt;
			break;
		}
	}
	if (!found)
	{
		return false;
	}

	VkPresentModeKHR selected_mode = VK_PRESENT_MODE_FIFO_KHR;
	for (auto& mode : supp_info->present_modes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			selected_mode = mode;
		}
	}
	this->present_mode = selected_mode;
	return true;
}

void VulkanSwapchain::destroy(VulkanContext* ctx)
{
	if (this->handle)
	{
		vkDestroySwapchainKHR(ctx->dev.log_dev, this->handle, ctx->vk_alloc);
		this->handle = null;
	}
}

// Images.

void VulkanImage::init(
	VulkanContext* ctx,
	VkFormat format,
	u32 width,
	u32 height,
	u32 depth,
	VkImageUsageFlags usage,
	VkImageAspectFlags aspect_flags,
	VkImageLayout init_layout,
	VkMemoryPropertyFlagBits mem_flags,
	VkImageType type,
	VkImageTiling tiling,
	VkSampleCountFlagBits sample_count,
	bool should_create_view
)
{
	VkImageCreateInfo image_ci = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = null,
		.imageType = type,
		.format = format,
		.extent = { .width = width, .height = height, .depth = depth, },
		.mipLevels = 4u,
		.arrayLayers = 1u,
		.samples = sample_count,
		.tiling = tiling,
		.usage = usage,
		.sharingMode = ctx->swapchain.sharing_mode,
		.initialLayout = init_layout,
	};

	VK_CHECK(vkCreateImage(ctx->dev.log_dev, &image_ci, ctx->vk_alloc, &this->handle));

	this->width = width;
	this->height = height;
	this->depth = depth;
	this->type = type;
	this->format = format;
	this->aspect_flags = aspect_flags;

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(ctx->dev.log_dev, this->handle, &mem_reqs);

	auto [mem_type_idx, is_acquired] = ctx->dev.find_mem_type_index(mem_reqs.memoryTypeBits, mem_flags);
	if (!is_acquired) panic("Failed to acquire memory type index");

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = null,
		.allocationSize = mem_reqs.size,
		.memoryTypeIndex = mem_type_idx,
	};

	VK_CHECK(vkAllocateMemory(ctx->dev.log_dev, &alloc_info, ctx->vk_alloc, &this->memory));
	VK_CHECK(vkBindImageMemory(ctx->dev.log_dev, this->handle, this->memory, 0));

	if (should_create_view)
	{
		this->create_view(ctx, aspect_flags);
	}
}

void VulkanImage::create_view(
	VulkanContext* ctx,
	VkImageAspectFlags aspect_flags
)
{
	this->view = create_view_from_raw_image(ctx, this->handle, this->format, (VkImageViewType)this->type, aspect_flags);
}

VkImageView create_view_from_raw_image(
	VulkanContext* ctx,
	VkImage raw_image,
	VkFormat format,
	VkImageViewType type,
	VkImageAspectFlags aspect_flags
)
{
	VkImageView view;

	VkImageViewCreateInfo image_view_ci = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = null,
		.image = raw_image,
		.viewType = type,
		.format = format,
		.subresourceRange = {
			.aspectMask = aspect_flags,
			.baseMipLevel = 0,
			.levelCount = VK_REMAINING_MIP_LEVELS,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	VK_CHECK(vkCreateImageView(ctx->dev.log_dev, &image_view_ci, ctx->vk_alloc, &view));
	return view;
}

void VulkanImage::cmd_transition_layout(
	VulkanCmdBuffer cmd_buff,
	VkImageLayout src_layout,
	VkImageLayout dest_layout,
	VkPipelineStageFlagBits2 src_stage_mask,
	VkPipelineStageFlagBits2 dest_stage_mask,
	VkAccessFlagBits2 src_access_mask,
	VkAccessFlagBits2 dest_access_mask
)
{
	cmd_transition_layout_raw_image(
		this->handle,
		cmd_buff,
		src_layout,
		dest_layout,
		src_stage_mask,
		dest_stage_mask,
		src_access_mask,
		dest_access_mask,
		(this->aspect_flags & VK_IMAGE_ASPECT_DEPTH_BIT) > 0
	);
}

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
)
{
	VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = src_stage_mask,
		.srcAccessMask = src_access_mask,
		.dstStageMask = dest_stage_mask,
		.dstAccessMask = dest_access_mask,
		.oldLayout = src_layout,
		.newLayout = dest_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = raw_image,
		.subresourceRange = {
			.aspectMask = is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = VK_REMAINING_MIP_LEVELS,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	VkDependencyInfo deps_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.dependencyFlags = 0,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier,
	};

	vkCmdPipelineBarrier2(cmd_buff.handle, &deps_info);
}

void VulkanImage::copy_data(
	VulkanCmdBuffer cmd_buff,
	const VulkanBufferCpu& src_buff,
	BufferChunkView buff_view,
	VkImageLayout dst_image_layout
)
{
	VkBufferImageCopy copy_region = {
		.bufferOffset = (u64)buff_view.offset,
		.imageSubresource = {
			.aspectMask = this->aspect_flags,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		.imageExtent = { .width = this->width, .height = this->height, .depth = this->depth }
	};

	vkCmdCopyBufferToImage(cmd_buff.handle, src_buff.handle, this->handle, dst_image_layout, 1, &copy_region);
}

void VulkanImage::destroy(VulkanContext* ctx)
{
	if (this->handle)
	{
		vkDestroyImageView(ctx->dev.log_dev, this->view, ctx->vk_alloc);
		vkFreeMemory(ctx->dev.log_dev, this->memory, ctx->vk_alloc);
		vkDestroyImage(ctx->dev.log_dev, this->handle, ctx->vk_alloc);
		this->view = null;
		this->memory = null;
		this->handle = null;
	}
}

// Synchronization primitives.

void VulkanSemaphore::init(VulkanContext* ctx)
{
    VkSemaphoreCreateInfo semaphore_ci = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VK_CHECK(vkCreateSemaphore(ctx->dev.log_dev, &semaphore_ci, ctx->vk_alloc, &this->handle));
}

void VulkanSemaphore::destroy(VulkanContext* ctx)
{
    if (this->handle)
    {
        vkDestroySemaphore(ctx->dev.log_dev, this->handle, ctx->vk_alloc);
        this->handle = null;
    }
}

void VulkanFence::init(VulkanContext* ctx, bool init_singaled)
{
    this->handle = (VkFence)null;
	this->is_signaled = init_singaled;
    VkFenceCreateInfo fence_ci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };
    if (init_singaled) {
        fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }
    VK_CHECK(vkCreateFence(ctx->dev.log_dev, &fence_ci, null, &this->handle));
}

bool VulkanFence::wait(VulkanContext* ctx)
{
    VkResult result = VK_TIMEOUT;

    while (true) {
        result = vkWaitForFences(ctx->dev.log_dev, 1u, &this->handle, VK_TRUE, ~u64(0));
        if (result != VK_TIMEOUT) break;
    }
    
    if (result == VK_SUCCESS) this->is_signaled = true;
    else
    {
        LOG_ERROR("Fence wait error occurred");
        return false;
    }
    return true;
}

void VulkanFence::reset(VulkanContext* ctx)
{
    if (!this->is_signaled) {
        return;
    }
    
    VK_CHECK(vkResetFences(ctx->dev.log_dev, 1u, &this->handle));
    this->is_signaled = false;
}

void VulkanFence::destroy(VulkanContext* ctx)
{
    if (this->handle) {
        vkDestroyFence(ctx->dev.log_dev, this->handle, ctx->vk_alloc);
        this->handle = null;
    }
}

// Buffers.

void VulkanBuffer::init(VulkanContext* ctx, sz capacity, const VulkanBufferConfig& config)
{
	VkBufferCreateInfo buff_ci = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = (u64)capacity,
		.usage = config.usage,
		.sharingMode = config.sharing_mode,
	};

	VK_CHECK(vkCreateBuffer(ctx->dev.log_dev, &buff_ci, ctx->vk_alloc, &this->handle));

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(ctx->dev.log_dev, this->handle, &mem_reqs);

	auto [mem_idx, is_acquired] = ctx->dev.find_mem_type_index(mem_reqs.memoryTypeBits, config.mem_props);
	if (!is_acquired) panic("Failed to acquire memory type index");

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = mem_reqs.size,
		.memoryTypeIndex = mem_idx,
	};

	VK_CHECK(vkAllocateMemory(ctx->dev.log_dev, &alloc_info, ctx->vk_alloc, &this->gpu_mem));
	VK_CHECK(vkBindBufferMemory(ctx->dev.log_dev, this->handle, this->gpu_mem, 0));

	this->capacity = mem_reqs.size;
}

void VulkanBuffer::realloc(sz new_capacity)
{
	EngineContext* engine_ctx = get_engine_context();
	VulkanContext* vk_ctx = &engine_ctx->vk_ctx;
	this->destroy(vk_ctx);
	this->init(vk_ctx, new_capacity, this->config);
}

void VulkanBuffer::destroy(VulkanContext* ctx)
{
	if (this->handle)
	{
		vkDestroyBuffer(ctx->dev.log_dev, this->handle, ctx->vk_alloc);
		this->handle = null;
	}
	if (this->gpu_mem)
	{
		vkFreeMemory(ctx->dev.log_dev, this->gpu_mem, ctx->vk_alloc);
		this->gpu_mem = null;
	}
}

// VulkanBufferCpu.

intern void vk_buffer_cpu_init_inner(VulkanBufferCpu* self, VulkanContext* ctx, sz capacity, const VulkanBufferConfig& config);
intern void vk_buffer_cpu_destroy_inner(VulkanBufferCpu* self, VulkanContext* ctx);
intern void vk_buff_cpu_realloc(VulkanBufferCpu* self, sz add_capacity);

void VulkanBufferCpu::init(VulkanContext* ctx, sz capacity, const VulkanBufferConfig& config)
{
	ASSERT_MSG((config.mem_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) > 0, "Must be HOST_VISIBLE");
	vk_buffer_cpu_init_inner(this, ctx, capacity, config);
	this->mutex.init();
}

intern void vk_buffer_cpu_init_inner(VulkanBufferCpu* self, VulkanContext* ctx, sz capacity, const VulkanBufferConfig& config)
{
	VkBufferCreateInfo buff_ci = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = (u64)capacity,
		.usage = config.usage,
		.sharingMode = config.sharing_mode,
	};

	VK_CHECK(vkCreateBuffer(ctx->dev.log_dev, &buff_ci, ctx->vk_alloc, &self->handle));

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(ctx->dev.log_dev, self->handle, &mem_reqs);

	u32 memory_index = ctx->dev.find_mem_type_index(mem_reqs.memoryTypeBits, config.mem_props);

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = mem_reqs.size,
		.memoryTypeIndex = memory_index,
	};

	VK_CHECK(vkAllocateMemory(ctx->dev.log_dev, &alloc_info, ctx->vk_alloc, &self->gpu_mem));
	VK_CHECK(vkBindBufferMemory(ctx->dev.log_dev, self->handle, self->gpu_mem, 0));

	self->capacity = mem_reqs.size;
	self->size = 0;
	self->config = config;

	ASSERT(map_vk_mem(&ctx->dev, &self->cpu_mem, self->gpu_mem, self->capacity));
}

BufferChunkView VulkanBufferCpu::append_data(Slice<Slice<u8>> data)
{
	sz data_len = 0;

	for (Slice<u8> chunk : data)
	{
		data_len += chunk.count;
	}

	// Critical part.
	this->mutex.lock();
	defer(this->mutex.unlock());

	sz diff = data_len - this->remain_mem();
	if (diff > 0) this->realloc(diff);

	sz offset = this->size;

	for (Slice<u8> chunk : data)
	{
		rg::mem_copy((u8*)this->cpu_mem + this->size, chunk.ptr, chunk.count);
		this->size += chunk.count;
	}

	return { offset, this->size - offset };
}

Maybe<BufferChunkView> VulkanBufferCpu::append_data_non_realloc(Slice<Slice<u8>> data)
{
	Maybe<BufferChunkView> res;
	sz data_len = 0;

	for (Slice<u8> chunk : data)
	{
		data_len += chunk.count;
	}

	sz diff = data_len - this->remain_mem();
	if (diff > 0) return res;

	sz offset = this->size;

	for (Slice<u8> chunk : data)
	{
		rg::mem_copy((char*)this->cpu_mem + this->size, chunk.ptr, chunk.count);
		this->size += chunk.count;
	}

	res.set_val({ offset, this->size - offset });
	return res;
}

intern void vk_buff_cpu_realloc(VulkanBufferCpu* self, sz add_capacity)
{
	EngineContext* engine_ctx = get_engine_context();
	VulkanContext* vk_ctx = &engine_ctx->vk_ctx;

	VulkanBufferCpu new_buff;
	vk_buffer_cpu_init_inner(&new_buff, vk_ctx, self->capacity + add_capacity, self->config);
	rg::mem_copy(new_buff.cpu_mem, self->cpu_mem, self->size);
	new_buff.size = self->size;
	vk_buffer_cpu_destroy_inner(self, vk_ctx);
	Mutex mutex = self->mutex;
	*self = new_buff;
	self->mutex = mutex;
}

void VulkanBufferCpu::destroy(VulkanContext* ctx)
{
	vk_buffer_cpu_destroy_inner(this, ctx);
	this->mutex.destroy();
}

intern void vk_buffer_cpu_destroy_inner(VulkanBufferCpu* self, VulkanContext* vk_ctx)
{
	if (self->handle)
	{
		vkDestroyBuffer(vk_ctx->dev.log_dev, self->handle, vk_ctx->vk_alloc);
		self->handle = null;
	}
	if (self->gpu_mem)
	{
		unmap_vk_mem(&vk_ctx->dev, self->gpu_mem);
		self->cpu_mem = null;
		vkFreeMemory(vk_ctx->dev.log_dev, self->gpu_mem, vk_ctx->vk_alloc);
		self->gpu_mem = null;
	}
}

// Command buffers.

void VulkanCmdBuffer::begin_recording(
	CmdBufferState* cmd_buff_state,
	VkCommandBufferUsageFlags flags
)
{
	VkCommandBufferBeginInfo begin_info = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = flags };
	VK_CHECK(vkBeginCommandBuffer(this->handle, &begin_info));
	*cmd_buff_state = CmdBufferState::RECORDING_BEGIN;
}

void cmd_buffer_begin_recording_many(
	Slice<VulkanCmdBuffer> cmd_buffs,
	Slice<CmdBufferState> cmd_buff_states,
	VkCommandBufferUsageFlags flags
)
{
	for (sz i = 0; i < cmd_buffs.count; ++i)
	{
          VulkanCmdBuffer cmd_buff = cmd_buffs[i];
          VkCommandBufferBeginInfo begin_info = {
              .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
              .flags = flags};
          VK_CHECK(vkBeginCommandBuffer(cmd_buff.handle, &begin_info));
          cmd_buff_states[i] = CmdBufferState::RECORDING_BEGIN;
	}
}

void VulkanCmdBuffer::end_recording(CmdBufferState* cmd_buff_state)
{
	VK_CHECK(vkEndCommandBuffer(this->handle));
	*cmd_buff_state = CmdBufferState::RECORDING_END;
}

void cmd_buffer_end_recording_many(Slice<VulkanCmdBuffer> cmd_buffs, Slice<CmdBufferState> cmd_buff_states)
{
	for (sz i = 0; i < cmd_buffs.count; ++i)
	{
		VulkanCmdBuffer cmd_buff = cmd_buffs[i];
		VK_CHECK(vkEndCommandBuffer(cmd_buff.handle));
		cmd_buff_states[i] = CmdBufferState::RECORDING_END;
	}
}

void VulkanCmdBuffer::submit(
	CmdBufferState* cmd_buff_state,
	VulkanContext* ctx,
	VkQueue queue,
	Slice<VulkanSemaphore> signal_semaphores,
	Slice<VulkanSemaphore> wait_semaphores,
	Slice<VkPipelineStageFlags> wait_dst_stage_masks,
	Maybe<VulkanFence> mb_fence
)
{
	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = (u32)wait_semaphores.count,
		.pWaitSemaphores = (VkSemaphore*)wait_semaphores.ptr,
		.pWaitDstStageMask = wait_dst_stage_masks.ptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &this->handle,
		.signalSemaphoreCount = (u32)signal_semaphores.count,
		.pSignalSemaphores = (VkSemaphore*)signal_semaphores.ptr,
	};

	auto [fence, has_fence] = mb_fence;

	if (!has_fence)
	{
		VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, null));
	}
	else
	{
		if (fence.is_signaled)
		{
			LOG_WARN("Fence can't be in signaled state when submitting a queue, resetting...");
			fence.reset(ctx);
		}
		VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, fence.handle));
	}

	*cmd_buff_state = CmdBufferState::SUBMITTED;
}

void cmd_buffer_submit_many(
	Slice<VulkanCmdBuffer> cmd_buffs,
	Slice<CmdBufferState> cmd_buff_states,
	VulkanContext* ctx,
	VkQueue queue,
	Slice<VulkanSemaphore> signal_semaphores,
	Slice<VulkanSemaphore> wait_semaphores,
	Slice<VkPipelineStageFlags> wait_dst_stage_masks,
	Maybe<VulkanFence> mb_fence
)
{
	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = (u32)wait_semaphores.count,
		.pWaitSemaphores = (VkSemaphore*)wait_semaphores.ptr,
		.pWaitDstStageMask = wait_dst_stage_masks.ptr,
		.commandBufferCount = (u32)cmd_buffs.count,
		.pCommandBuffers = (VkCommandBuffer*)cmd_buffs.ptr,
		.signalSemaphoreCount = (u32)signal_semaphores.count,
		.pSignalSemaphores = (VkSemaphore*)signal_semaphores.ptr,
	};

	auto [fence, has_fence] = mb_fence;

	if (!has_fence)
	{
		VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, null));
	}
	else
	{
		if (fence.is_signaled)
		{
			LOG_WARN("Fence can't be in signaled state when submitting a queue, resetting...");
			fence.reset(ctx);
		}
		VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, fence.handle));
	}

	for (CmdBufferState& cmd_buff_state : cmd_buff_states)
	{
		cmd_buff_state = CmdBufferState::SUBMITTED;
	}
}

void VulkanCmdBuffer::reset(CmdBufferState* cmd_buff_state, VkCommandBufferResetFlags flags)
{
	VK_CHECK(vkResetCommandBuffer(this->handle, flags));
	*cmd_buff_state = CmdBufferState::READY;
}

void cmd_buffer_reset_many(
	Slice<VulkanCmdBuffer> cmd_buffs,
	Slice<CmdBufferState> cmd_buff_states,
	VkCommandBufferResetFlags flags
)
{
	for (sz i = 0; i < cmd_buffs.count; ++i)
	{
		VulkanCmdBuffer cmd_buff = cmd_buffs[i];
		VK_CHECK(vkResetCommandBuffer(cmd_buff.handle, flags));
		cmd_buff_states[i] = CmdBufferState::READY;
	}
}

void cmd_buffer_end_submit_reset_many(
	Slice<VulkanCmdBuffer> cmd_buffs,
	Slice<CmdBufferState> cmd_buff_states,
	VulkanContext* vk_ctx,
	VkQueue queue,
	bool wait_on_queue
)
{
	cmd_buffer_end_recording_many(cmd_buffs, cmd_buff_states);
	cmd_buffer_submit_many(cmd_buffs, cmd_buff_states, vk_ctx, queue);
	if (wait_on_queue) VK_CHECK(vkQueueWaitIdle(queue));
	cmd_buffer_reset_many(cmd_buffs, cmd_buff_states);
}

void cmd_copy_data_between_buff(
	VulkanCmdBuffer cmd_buff,
	VulkanBuffer src,
	VulkanBuffer dst,
	VkBufferCopy* copy_region
)
{
	vkCmdCopyBuffer(cmd_buff.handle, src.handle, dst.handle, 1, copy_region);
}

void cmd_copy_data_buff_to_img(
	VulkanCmdBuffer cmd_buff,
	VulkanBuffer src_buffer,
	VulkanImage* dst_image,
	VkImageLayout dst_image_layout
)
{
	VkBufferImageCopy copy_region = {
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		.imageExtent = { .width = dst_image->width, .height = dst_image->height, .depth = dst_image->depth }
	};

	vkCmdCopyBufferToImage(cmd_buff.handle, src_buffer.handle, dst_image->handle, dst_image_layout, 1, &copy_region);
}

void cmd_bind_descriptor_sets(
	VulkanCmdBuffer cmd_buff,
	VulkanPipeline* pipeline,
	Slice<VkDescriptorSet> descriptor_sets,
	u32 start_index = 0
)
{
	vkCmdBindDescriptorSets(
		cmd_buff.handle,
		pipeline->bind_point,
		pipeline->layout,
		start_index,
		descriptor_sets.count,
		descriptor_sets.ptr,
		0,
		null
	);
}

#ifdef RG_DEBUG
void cmd_buff_assert_state(Slice<CmdBufferState> cmd_buff_states, CmdBufferState expected, CString error_msg)
{
	for (CmdBufferState state : cmd_buff_states)
	{
		ASSERT_MSG(state == expected, error_msg);
	}
}
#endif // RG_DEBUG

// Pool

void VulkanCmdPool::init(VulkanContext* ctx, u8 queue_fam_idx)
{
	VkCommandPoolCreateInfo cmd_pool_ci = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = (u32)queue_fam_idx,
	};
	VK_CHECK(vkCreateCommandPool(ctx->dev.log_dev, &cmd_pool_ci, ctx->vk_alloc, &this->handle));
}

void VulkanCmdPool::allocate_cmd_buffers(
	VulkanDevice* dev,
	Slice<VulkanCmdBuffer> out_cmd_buffs,
	bool is_primary
)
{
	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = this->handle,
		.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY,
		.commandBufferCount = (u32)out_cmd_buffs.count,
	};
	VK_CHECK(vkAllocateCommandBuffers(dev->log_dev, &alloc_info, (VkCommandBuffer*)out_cmd_buffs.ptr));
}

void VulkanCmdPool::destroy_cmd_buffers(VulkanDevice* dev, Slice<VulkanCmdBuffer> out_cmd_buffs)
{
	vkFreeCommandBuffers(dev->log_dev, this->handle, out_cmd_buffs.count, (VkCommandBuffer*)out_cmd_buffs.ptr);
}

void VulkanCmdPool::destroy(VulkanContext* ctx)
{
	if (this->handle)
	{
		vkDestroyCommandPool(ctx->dev.log_dev, this->handle, ctx->vk_alloc);
		this->handle = null;
	}
}

// Shader and pipeline.

intern VkVertexInputBindingDescription get_binding_descr(u8 binding_index);

VulkanShader create_shader(VulkanContext* ctx, StrView file_name, VulkanShaderConfig* config)
{
	VulkanShader shader;
	shader.init(ctx, file_name, config);
	return shader;
}

void VulkanShader::init(VulkanContext* ctx, StrView file_name, VulkanShaderConfig* config)
{
	this->ctx = ctx;
	this->mod.init(ctx, file_name, config->stage_bits);
	this->stage_bits = config->stage_bits;

	this->init_descriptor_state(ctx, config);

	FArray<VkDescriptorSetLayout, MAX_DESCRIPTOR_LAYOUT_COUNT> layouts;

	for (VulkanDescriptorSetLayout layout : this->descriptor_layouts)
	{
		layouts.push(layout.handle);
	}

	VkPipelineLayoutCreateInfo pipeline_layouts_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = (u32)layouts.count,
		.pSetLayouts = layouts.data,
		.pushConstantRangeCount = (u32)config->push_constant_ranges.count,
		.pPushConstantRanges = config->push_constant_ranges.ptr,
	};

	// Will be shared across all pipelines for this shader.
	VkPipelineLayout pipeline_layout;
	VK_CHECK(vkCreatePipelineLayout(ctx->dev.log_dev, &pipeline_layouts_ci, ctx->vk_alloc, &pipeline_layout));

	this->pipelines.resize(config->pipeline_configs.count);

	for (VulkanPipeline& pipe : this->pipelines)
	{
		pipe.layout = pipeline_layout;
	}

	for (u32 i = 0; i < config->pipeline_configs.count; ++i)
	{
		VulkanPipelineConfig* pipe_config = &config->pipeline_configs[i];
		this->create_pipeline(
			&this->pipelines[i],
			ctx,
			pipe_config,
			config->attrib_descriptions,
			config->vertex_binding_index
		);
	}
}

void VulkanShader::create_pipeline(
	VulkanPipeline* out_pipeline,
	VulkanContext* ctx,
	VulkanPipelineConfig* config,
	Slice<VkVertexInputAttributeDescription> attrib_descriptions,
	u8 vertex_bind_ind
)
{
	EngineContext* engine_ctx = get_engine_context();
	Array<VkDynamicState, 2> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamic_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = (u32)dynamic_states.len(),
		.pDynamicStates = dynamic_states.data,
	};

	VkVertexInputBindingDescription binding_descr = get_binding_descr(vertex_bind_ind);

	VkPipelineVertexInputStateCreateInfo vertex_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &binding_descr,
		.vertexAttributeDescriptionCount = (u32)attrib_descriptions.count,
		.pVertexAttributeDescriptions = attrib_descriptions.ptr,
	};

	VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = config->primitive_topology,
	};

	VkPipelineViewportStateCreateInfo viewport_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &engine_ctx->renderer.viewport,
		.scissorCount = 1,
		.pScissors = &engine_ctx->renderer.scissors
	};

	VkPipelineRasterizationStateCreateInfo rasterization_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = config->polygon_mode,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = config->line_width,
	};

	VkPipelineMultisampleStateCreateInfo multisample_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = config->sample_count,
		.sampleShadingEnable = VK_FALSE,
	};

	VkPipelineColorBlendAttachmentState color_blending_attachment = {
		.blendEnable = (VkBool32)config->enable_color_blend,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
			| VK_COLOR_COMPONENT_A_BIT,
	};

	VkPipelineColorBlendStateCreateInfo color_blending_state = {
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &color_blending_attachment,
	};

	VkPipelineRenderingCreateInfo rendering_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &ctx->swapchain.surface_fmt.format,
		.depthAttachmentFormat = ctx->dev.depth_fmt,
	};

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = null,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS, // lower depth => write pixel
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
	};

	FArray<VkPipelineShaderStageCreateInfo, (sz)ShaderStageKind::EnumSize> shader_stages;
	this->mod.get_pipeline_stages(&shader_stages);

	VkGraphicsPipelineCreateInfo pipeline_ci = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &rendering_ci,
		.stageCount = (u32)shader_stages.count,
		.pStages = shader_stages.data,
		.pVertexInputState = &vertex_ci,
		.pInputAssemblyState = &input_assembly_ci,
		.pViewportState = &viewport_ci,
		.pRasterizationState = &rasterization_ci,
		.pMultisampleState = &multisample_ci,
		.pDepthStencilState = &depth_stencil_state,
		.pColorBlendState = &color_blending_state,
		.pDynamicState = &dynamic_state,
		.layout = out_pipeline->layout,
		.renderPass = null,
		.basePipelineHandle = (VkPipeline)null,
		.basePipelineIndex = -1,
	};

	VK_CHECK(vkCreateGraphicsPipelines(ctx->dev.log_dev, null, 1, &pipeline_ci, ctx->vk_alloc, &out_pipeline->handle));
	out_pipeline->bind_point = config->bind_point;

	LOG_INFO("Pipeline was successfully created!");
}

void VulkanShader::init_descriptor_state(VulkanContext* ctx, VulkanShaderConfig* conf)
{
	this->max_entities = conf->max_entities_per_frame;

	// create layout for each set
	for (auto& set_info : conf->descriptor_set_infos)
	{
		this->descriptor_layouts.push(create_descriptor_set_layout(ctx, set_info.bindings));
		for (VkDescriptorType type : set_info.types)
		{
			this->pool_sizes.push({ type, this->max_entities * (u32)FRAMES_IN_FLIGHT });
		}
	}

	Allocator* persist_alloc = get_persist_allocator();
	this->descriptor_pools.init_capacity(persist_alloc, INIT_POOL_COUNT);
	this->allocate_pool(ctx);
}

VulkanDescriptorPool* VulkanShader::allocate_pool(VulkanContext* ctx)
{
	this->descriptor_pools.push(
		create_descriptor_pool(ctx, this->pool_sizes.slice(), this->max_entities * FRAMES_IN_FLIGHT)
	);
	return this->descriptor_pools.last_ref();
}

// VulkanPipeline.

void VulkanShader::cmd_bind_curr_pipeline(VulkanCmdBuffer cmd_buffer)
{
	VulkanPipeline* pipeline = this->get_curr_pipeline();
	pipeline->cmd_bind(cmd_buffer);
}

void VulkanPipeline::cmd_bind(VulkanCmdBuffer cmd_buffer)
{
	vkCmdBindPipeline(cmd_buffer.handle, this->bind_point, this->handle);
}

EntityShaderState VulkanShader::allocate_entity_resources(VulkanContext* ctx)
{
	// Doubling layouts for each frame in flight.
	FArray<VkDescriptorSetLayout, MAX_DESCRIPTOR_LAYOUT_COUNT * FRAMES_IN_FLIGHT> all_layouts;
	u32 i = 0;

	for (i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		for (const VulkanDescriptorSetLayout& layout : this->descriptor_layouts)
		{
			all_layouts.push(layout.handle);
		}
	}

	i = 0;
	for (i = 0; i < this->descriptor_pools.count; ++i)
	{
		VulkanDescriptorPool* pool = &this->descriptor_pools[i];
		auto [alloc_res, is_success] = pool->allocate_sets(&ctx->dev, all_layouts.slice());
		if (!is_success) continue;

		EntityShaderState res;
		res.pool_idx = (u16)i;
		res.sets_count = (u16)alloc_res.count;
		res.set_sets_start_idx((u32)alloc_res.start_index);
		return res;
	}

	VulkanDescriptorPool* new_pool = this->allocate_pool(ctx);
	auto [alloc_res, is_success] = new_pool->allocate_sets(&ctx->dev, all_layouts.slice());
	ASSERT_MSG(is_success, "Can't allocate descriptor sets from the new pool, probably internal error");

	EntityShaderState res;
	res.pool_idx = (u16)i;
	res.sets_count = (u16)alloc_res.count;
	res.set_sets_start_idx((u32)alloc_res.start_index);
	return res;
}

Slice<VkDescriptorSet> VulkanShader::get_entity_resources(EntityShaderState entity_state, sz curr_frame)
{
	Slice<VkDescriptorSet> sets = this->descriptor_pools[entity_state.pool_idx].get_sets(
		entity_state.sets_start_idx(),
		entity_state.sets_count
	);
	return sets.slice(curr_frame * this->descriptor_layouts.count, this->descriptor_layouts.count);
}

void VulkanShader::destroy_entity_resources(VulkanContext* ctx, EntityShaderState entity_state)
{
	this->descriptor_pools[entity_state.pool_idx].free_sets(
		ctx,
		entity_state.sets_start_idx(),
		entity_state.sets_count
	);
}

void VulkanShader::update_entity_resources(
	VulkanContext* ctx,
	EntityShaderState entity_state,
	Slice<DescriptorUpdateInfo> update_infos,
	sz curr_frame
)
{
	Slice<VkDescriptorSet> entity_sets = this->get_entity_resources(entity_state, curr_frame);
	FArray<VkWriteDescriptorSet, MAX_DESCRIPTOR_BINDING_COUNT> writes;

	for (u32 i = 0; i < update_infos.count; ++i)
	{
		const DescriptorUpdateInfo& update_info = update_infos[i];
		VkWriteDescriptorSet write = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = entity_sets[update_info.set_index],
			.dstBinding = i,
			.descriptorCount = 1,
			.descriptorType = update_info.type,
		};

		switch (update_info.type)
		{
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				write.pImageInfo = &update_info.image_info;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				write.pBufferInfo = &update_info.buff_info;
			case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
				write.pTexelBufferView = update_info.texel_buff_info;
			default:
				unreachable("Unknown descriptor set update info type: %s", update_info.type);
		}
		writes.push(write);
	}

	Slice<VkWriteDescriptorSet> writes_view = writes.slice();
	vkUpdateDescriptorSets(ctx->dev.log_dev, writes_view.count, writes_view.ptr, 0, null);
}

void VulkanShader::cmd_bind_entity_resources(
	VulkanCmdBuffer cmd_buff,
	EntityShaderState entity_state,
	VulkanPipeline* pipeline,
	sz curr_frame,
	sz first_set
)
{
	Slice<VkDescriptorSet> entity_sets = this->get_entity_resources(entity_state, curr_frame);
	cmd_bind_descriptor_sets(cmd_buff, pipeline, entity_sets);
}

// Descriptor pool.

VulkanDescriptorPool create_descriptor_pool(VulkanContext* ctx, Slice<VkDescriptorPoolSize> pool_sizes, u32 max_sets)
{
	VulkanDescriptorPool pool;
	pool.init(ctx, pool_sizes, max_sets);
	return pool;
}

void VulkanDescriptorPool::init(VulkanContext* ctx, Slice<VkDescriptorPoolSize> pool_sizes, u32 max_sets)
{
	VkDescriptorPoolCreateInfo descriptor_pool_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = max_sets,
		.poolSizeCount = (u32)pool_sizes.count,
		.pPoolSizes = pool_sizes.ptr,
	};
	VK_CHECK(vkCreateDescriptorPool(ctx->dev.log_dev, &descriptor_pool_ci, ctx->vk_alloc, &this->handle));
	Allocator* p_alloc = get_persist_allocator();
	this->sets.init_capacity(p_alloc, max_sets);
}

Maybe<PoolAllocationResult> VulkanDescriptorPool::allocate_sets(VulkanDevice* dev, Slice<VkDescriptorSetLayout> layouts)
{
	Maybe<PoolAllocationResult> res;
	if (this->sets.count + layouts.count > this->sets.capacity)
	{
		return res;
	}

	VkDescriptorSetAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = this->handle,
		.descriptorSetCount = (u32)layouts.count,
		.pSetLayouts = (VkDescriptorSetLayout*)layouts.ptr,
	};

	VkDescriptorSet* allocated_sets = this->sets.data + this->sets.count;
	VK_CHECK(vkAllocateDescriptorSets(dev->log_dev, &alloc_info, allocated_sets));
	res.set_val({ .start_index = this->sets.count, .count = layouts.count });
	this->sets.count += layouts.count;
	return res;
}

Slice<VkDescriptorSet> VulkanDescriptorPool::get_sets(sz index, sz len)
{
	return this->sets.slice(index, len);
}

void VulkanDescriptorPool::free_sets(VulkanContext* ctx, u32 start, ushort len)
{
	Slice<VkDescriptorSet> sets_to_free = this->sets.slice(start, len);
	VK_CHECK(vkFreeDescriptorSets(ctx->dev.log_dev, this->handle, sets_to_free.count, sets_to_free.ptr));
}

// Descriptor set layout.

VulkanDescriptorSetLayout create_descriptor_set_layout(VulkanContext* ctx, Slice<VkDescriptorSetLayoutBinding> bindings)
{
	VulkanDescriptorSetLayout layout;
	layout.init(ctx, bindings);
	return layout;
}

void VulkanDescriptorSetLayout::init(VulkanContext* ctx, Slice<VkDescriptorSetLayoutBinding> bindings)
{
	VkDescriptorSetLayoutCreateInfo layout_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = (u32)bindings.count,
		.pBindings = bindings.ptr,
	};
	this->bindings.push(bindings);
	VK_CHECK(vkCreateDescriptorSetLayout(ctx->dev.log_dev, &layout_ci, ctx->vk_alloc, &this->handle));
}

void init_descriptor_bindings(
	VkShaderStageFlags stage_flags,
	Slice<VkDescriptorType> descriptor_types,
	Slice<VkDescriptorSetLayoutBinding> out_bindings
)
{
	for (u32 i = 0; i < out_bindings.count; ++i)
	{
		VkDescriptorSetLayoutBinding* binding = &out_bindings[i];
		binding->binding = i;
		binding->descriptorType = descriptor_types[i];
		binding->descriptorCount = 1u;
		binding->stageFlags = stage_flags;
		binding->pImmutableSamplers = null;
	}
}

// Shader module.

bool VulkanShaderModule::init(VulkanContext* ctx, StrView file_name, BitInt<u8> stage_bits)
{
	Arena* temp_alloc = get_temp_allocator();
	TEMP_ALLOC_SCOPE(temp_alloc);

	Path file_path;
	FArray<StrView, 2> path_parts = { SHADERS_PATH, file_name };
	file_path.init(temp_alloc, path_parts.slice());

	auto [file_contents, success] = rg::file_read(temp_alloc, &file_path);

	if (!success)
	{
		LOG_ERROR("Shader file read failed, path: " FMT_PLACEHOLDER_LEN, FMT_DSTRING_VAL(file_path));
		return false;
	}

	VkShaderModuleCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = (u32)file_contents.count,
		.pCode = (u32*)file_contents.data,
	};

	this->stage_bits = stage_bits;

	VK_CHECK(vkCreateShaderModule(ctx->dev.log_dev, &create_info, ctx->vk_alloc, &this->handle));
	return true;
}

void VulkanShaderModule::get_pipeline_stages(
	FArray<VkPipelineShaderStageCreateInfo, (sz)ShaderStageKind::EnumSize>* out_pipeline_stages
)
{
	ASSERT_MSG(out_pipeline_stages->count == 0, "Must be zero elements in input");

	VkPipelineShaderStageCreateInfo stage_ci{};

	if (this->stage_bits.is_set((sz)ShaderStageKind::VERTEX))
	{
		stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_ci.flags |= VK_SHADER_STAGE_VERTEX_BIT;
		stage_ci.module = this->handle;
		stage_ci.pName = VERTEX_SHADER_ENTRY_NAME.ptr;

		out_pipeline_stages->push(stage_ci);
		rg::mem_zero(&stage_ci, sizeof(stage_ci));
	}
	if (this->stage_bits.is_set((sz)ShaderStageKind::FRAGMENT))
	{
		stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_ci.flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_ci.module = this->handle;
		stage_ci.pName = FRAGMENT_SHADER_ENTRY_NAME.ptr;

		out_pipeline_stages->push(stage_ci);
		rg::mem_zero(&stage_ci, sizeof(stage_ci));
	}
	if (this->stage_bits.is_set((sz)ShaderStageKind::COMPUTE))
	{
		stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_ci.flags |= VK_SHADER_STAGE_COMPUTE_BIT;
		stage_ci.module = this->handle;
		stage_ci.pName = COMPUTE_SHADER_ENTRY_NAME.ptr;

		out_pipeline_stages->push(stage_ci);
		rg::mem_zero(&stage_ci, sizeof(stage_ci));
	}
	if (this->stage_bits.is_set((sz)ShaderStageKind::GEOMETRY))
	{
		stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_ci.flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
		stage_ci.module = this->handle;
		stage_ci.pName = GEOMETRY_SHADER_ENTRY_NAME.ptr;

		out_pipeline_stages->push(stage_ci);
		rg::mem_zero(&stage_ci, sizeof(stage_ci));
	}
	if (this->stage_bits.is_set((sz)ShaderStageKind::TESSELLATION_CONTROL))
	{
		stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_ci.flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		stage_ci.module = this->handle;
		stage_ci.pName = TESSELATION_CONTROL_SHADER_ENTRY_NAME.ptr;

		out_pipeline_stages->push(stage_ci);
		rg::mem_zero(&stage_ci, sizeof(stage_ci));
	}
	if (this->stage_bits.is_set((sz)ShaderStageKind::TESSELLATION_EVAL))
	{
		stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_ci.flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		stage_ci.module = this->handle;
		stage_ci.pName = TESSELATION_EVAL_SHADER_ENTRY_NAME.ptr;
		out_pipeline_stages->push(stage_ci);
	}
}

intern VkVertexInputBindingDescription get_binding_descr(u8 binding_index)
{
	return { .binding = (u32)binding_index, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, };
}

// Pipeline destroy's.

void VulkanShader::destroy(VulkanContext* ctx)
{
	for (auto& layout : this->descriptor_layouts)
	{
		layout.destroy(ctx);
	}
	for (auto& pipe : this->pipelines)
	{
		pipe.destroy(ctx);
	}
	for (auto &pool : this->descriptor_pools)
	{
		pool.destroy(ctx);
	}
	this->mod.destroy(ctx);
}

void VulkanPipeline::destroy(VulkanContext* ctx)
{
	if (this->handle)
	{
		vkDestroyPipeline(ctx->dev.log_dev, this->handle, ctx->vk_alloc);
		this->handle = null;
	}
}

void VulkanShaderModule::destroy(VulkanContext* ctx)
{
	if (this->handle)
	{
		vkDestroyShaderModule(ctx->dev.log_dev, this->handle, ctx->vk_alloc);
		this->handle = null;
	}
}

void VulkanDescriptorSetLayout::destroy(VulkanContext* ctx)
{
	if (this->handle)
	{
		vkDestroyDescriptorSetLayout(ctx->dev.log_dev, this->handle, ctx->vk_alloc);
		this->handle = null;
	}
}

void VulkanDescriptorPool::destroy(VulkanContext* ctx)
{
	if (this->handle)
	{
		vkDestroyDescriptorPool(ctx->dev.log_dev, this->handle, ctx->vk_alloc);
		this->handle = null;
	}
}

// Utilities.

bool map_vk_mem(VulkanDevice* dev, void** dest, VkDeviceMemory src, sz size_bytes, sz offset, VkMemoryMapFlags flags)
{
	if (vkMapMemory(
		dev->log_dev,
		src,
		offset,
		size_bytes,
		flags,
		dest
	)
		!= VK_SUCCESS)
	{
		return false;
	}
	return true;
}

void unmap_vk_mem(VulkanDevice* dev, VkDeviceMemory gpu_mem)
{
	vkUnmapMemory(dev->log_dev, gpu_mem);
}

} // rg
