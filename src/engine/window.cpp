#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "engine/entry.hpp"

namespace rg
{

intern void glfw_mouse_move_callback(GLFWwindow* window, f64 x, f64 y);
intern void glfw_mouse_btn_callback(
	GLFWwindow* window,
	s32 btn,
	s32 action,
	s32 mods
);
intern void glfw_key_callback(
	GLFWwindow* window,
	s32 key,
	s32 scancode,
	s32 action,
	s32 mode
);
intern void glfw_mouse_wheel_callback(
	GLFWwindow* window,
	double xoffset,
	double yoffset
);

bool Window::init(AppConfig config)
{
	if (!glfwInit()) return false;
	if (!glfwVulkanSupported()) return false;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* handle = glfwCreateWindow(
		(s32)config.window_width,
		(s32)config.window_height,
		config.window_name,
		null,
		null
	);

	if (!handle) return false;

	this->handle = handle;
	this->width = config.window_width;
	this->height = config.window_height;

	glfwSetWindowUserPointer(this->handle, null);
	glfwSetCursorPosCallback(this->handle, &glfw_mouse_move_callback);
	glfwSetMouseButtonCallback(this->handle, &glfw_mouse_btn_callback);
	glfwSetKeyCallback(this->handle, &glfw_key_callback);
	glfwSetScrollCallback(this->handle, &glfw_mouse_wheel_callback);

#ifdef RG_HIDE_CURSOR
	glfwSetInputMode(window.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#endif

	return true;
}

intern void glfw_mouse_move_callback(GLFWwindow* window, f64 x, f64 y)
{
	MousePos pos;
	pos.x = (float)x;
	pos.y = (float)y;
	EngineContext* engine_ctx = get_engine_context();
	engine_ctx->input_sys.process_mouse_move(pos);
}

intern void glfw_mouse_btn_callback(
	GLFWwindow* window,
	s32 btn,
	s32 action,
	s32 mods
)
{
	EngineContext* engine_ctx = get_engine_context();
	engine_ctx->input_sys.process_mouse_button(
		(MouseButton)btn,
		action == GLFW_PRESS
	);
}

intern void glfw_key_callback(
	GLFWwindow* window,
	s32 key,
	s32 scancode,
	s32 action,
	s32 mode
)
{
	// We're handling repeat outselves.
	if (action == GLFW_REPEAT) return;
	EngineContext* engine_ctx = get_engine_context();
	engine_ctx->input_sys.process_key((sz)key, action == GLFW_PRESS);
}

intern void glfw_mouse_wheel_callback(
	GLFWwindow* window,
	double xoffset,
	double yoffset
)
{
	EngineContext* engine_ctx = get_engine_context();
	engine_ctx->input_sys.process_mouse_wheel(yoffset < 0);
}

void Window::create_vk_surface(VulkanContext* vk_ctx, VkSurfaceKHR* surface)
{
	VK_CHECK(glfwCreateWindowSurface(vk_ctx->instance, this->handle, vk_ctx->vk_alloc, surface));
}

VkExtent2D Window::get_screen_coordinates()
{
	VkExtent2D coords;
	glfwGetFramebufferSize(
		this->handle,
		(s32*)&coords.width,
		(s32*)&coords.height
	);
	return coords;
}

void Window::destroy()
{
	if (this->handle)
	{
		glfwDestroyWindow(this->handle);
		this->handle = null;
		glfwTerminate();
	}
}
 
} // rg

