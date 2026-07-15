#include <vulkan/vulkan.h>
#include "engine/entry.hpp"
#include "engine/window.hpp"

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

Maybe<Window> create(CString name, s32 width, s32 height)
{
    Maybe<Window> wnd;

	if (!glfwInit())
	{
		return wnd;
	}
	if (!glfwVulkanSupported())
	{
		return wnd;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* handle = glfwCreateWindow(
		(s32)width,
		(s32)height,
		name,
		null,
		null
	);

	if (!handle)
	{
		return wnd;
	}

	Window window = { .handle = handle, .width = width, .height = height };

	glfwSetWindowUserPointer(window.handle, null);
	glfwSetCursorPosCallback(window.handle, &glfw_mouse_move_callback);
	glfwSetMouseButtonCallback(window.handle, &glfw_mouse_btn_callback);
	glfwSetKeyCallback(window.handle, &glfw_key_callback);
	glfwSetScrollCallback(window.handle, &glfw_mouse_wheel_callback);

#ifdef RG_HIDE_CURSOR
	glfwSetInputMode(window.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#endif

	return wnd;
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

void Window::create_vk_surface(VkInstance instance, VkSurfaceKHR* surface)
{
	glfwCreateWindowSurface(instance, this->handle, null, surface);
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

