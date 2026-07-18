#ifndef _RG_ENTRY_HPP_
#define _RG_ENTRY_HPP_

#include "core/basic.hpp"
#include "engine/event.hpp"
#include "engine/input.hpp"
#include "engine/vk_core.hpp"
#include "engine/window.hpp"
#include "engine/renderer.hpp"

namespace rg
{

void application_init();
void application_run();
void application_destroy();

// Engine context.

struct EngineContext
{
    EventSystem event_sys;
    InputSystem input_sys;
    VulkanContext vk_ctx;
    Renderer renderer;
    Window window;
    
    void init();
    void destroy();
};

EngineContext* get_engine_context();

constexpr sz FRAMES_IN_FLIGHT = 2;
constexpr VkClearDepthStencilValue CLEAR_DEPTH_VALUE = { .depth = 1.0f, .stencil = 0 };
constexpr VkOffset2D OFFSET_START = { 0, 0 };
#ifdef RG_DEBUG
const StrView ASSETS_PATH                            = { CSTR_SIZED("debug/assets/") };
const StrView SHADERS_PATH                           = { CSTR_SIZED("debug/shaders/") };
const StrView TEXTURES_PATH                          = { CSTR_SIZED("debug/assets/textures/") };
const StrView MODELS_PATH                            = { CSTR_SIZED("debug/assets/models/") };
#else
const StrView ASSETS_PATH                            = { CSTR_SIZED("release/assets/") };
const StrView SHADERS_PATH                           = { CSTR_SIZED("release/shaders/") };
const StrView TEXTURES_PATH                          = { CSTR_SIZED("release/assets/textures/") };
const StrView MODELS_PATH                            = { CSTR_SIZED("release/assets/models/") };
#endif
const StrView VERTEX_SHADER_ENTRY_NAME              = { CSTR_SIZED("vertMain") };
const StrView FRAGMENT_SHADER_ENTRY_NAME            = { CSTR_SIZED("fragMain") };
const StrView COMPUTE_SHADER_ENTRY_NAME             = { CSTR_SIZED("compMain") };
const StrView GEOMETRY_SHADER_ENTRY_NAME            = { CSTR_SIZED("geoMain") };
const StrView TESSELATION_CONTROL_SHADER_ENTRY_NAME = { CSTR_SIZED("tesControlMain") };
const StrView TESSELATION_EVAL_SHADER_ENTRY_NAME    = { CSTR_SIZED("tesEvalMain") };

} // rg

#endif // _RG_ENTRY_HPP_
