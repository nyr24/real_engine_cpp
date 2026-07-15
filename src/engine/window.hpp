#ifndef _RG_WINDOW_HPP_
#define _RG_WINDOW_HPP_

#include <vulkan/vulkan.h>
#include "core/basic.hpp"
#include "glfw/glfw3.h"

namespace rg
{

struct Window
{
    GLFWwindow* handle;
    s32 width;
    s32 height;

    static Maybe<Window> create(CString name, s32 width, s32 height);
    void create_vk_surface(VkInstance instance, VkSurfaceKHR* surface);
    VkExtent2D get_screen_coordinates();
    void destroy();
};

} // rg

#endif // _RG_WINDOW_HPP_
