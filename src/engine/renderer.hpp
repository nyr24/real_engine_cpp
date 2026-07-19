#ifndef _RG_RENDERER_HPP_
#define _RG_RENDERER_HPP_

#include <vulkan/vulkan.h>
#include "core/clock.hpp"
#include "core/math.hpp"
#include "engine/vk_core.hpp"
#include "engine/entity.hpp"

namespace rg
{

struct Renderer
{
    static constexpr sz INIT_ENTITY_COUNT            = 1 << 7;  // 128
    static constexpr sz INIT_ELEM_BUFF_CAPACITY      = 1 << 22; // 4MB
    static constexpr sz MAX_BACKGROUND_TASKS         = 1 << 6;  // 64
    static constexpr u8 MASK_EXIT_SCHEDULED          = 0b1;
    static constexpr u8 MASK_RESIZE_SCHEDULED        = 0b10;

	DArray<Entity> entities;
	Mat4 view;
	Mat4 proj;
	// ElementBuffer elem_buff;
	// Camera camera;
	Clock frame_clock;
	VkViewport viewport;
	VkRect2D scissors;
	// ColorRGBA clear_color;
	// RingBuffer{BackgroundTask, MAX_BACKGROUND_TASKS} background_tasks;
	// [exit_scheduled=1;resize_scheduled=0]
	BitInt<u8> bits;

    void init(VulkanContext* vk_ctx);
    void update_viewport_scissors(VkExtent2D new_extent);
    VkExtent2D get_extent(); 
    void destroy();

    bool exit_scheduled() { return this->bits.get_mask(MASK_EXIT_SCHEDULED); }
    void set_exit_scheduled(bool new_exit_scheduled)
    {
        this->bits.set_mask((u8)new_exit_scheduled, MASK_EXIT_SCHEDULED);
    }
    bool resize_scheduled() { return this->bits.get_mask(MASK_RESIZE_SCHEDULED); }
    void set_resize_scheduled(bool new_resize_scheduled)
    {
        this->bits.set_mask((u8)new_resize_scheduled, MASK_RESIZE_SCHEDULED);
    }
};

} // rg

#endif // _RG_RENDERER_HPP_
