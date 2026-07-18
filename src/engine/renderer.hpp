#ifndef _RG_RENDERER_HPP_
#define _RG_RENDERER_HPP_

#include <vulkan/vulkan.h>

namespace rg
{

struct Renderer
{
    VkRect2D scissors;
    VkViewport viewport;

    void update_viewport_scissors(VkExtent2D new_extent);
    VkExtent2D get_extent(); 
};

} // rg

#endif // _RG_RENDERER_HPP_
