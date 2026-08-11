#ifndef _RG_RENDERER_HPP_
#define _RG_RENDERER_HPP_

#include "volk/volk.h"
#include "core/clock.hpp"
#include "core/math.hpp"
#include "collections/darray.hpp"
#include "engine/vk_core.hpp"
#include "engine/shared.hpp"
#include "vk_core.hpp"

namespace rg
{

constexpr f32 MOUSE_SENSIVITY_SLOW       = 0.0005f;
constexpr f32 MOUSE_SENSIVITY_MEDIUM     = 0.0008f;
constexpr f32 MOUSE_SENSIVITY_FAST       = 0.001f;
constexpr f32 MOUSE_SENSIVITY_SUPER_FAST = 0.0025f;

const Vec3 DIRECTION_RIGHT  = { 1, 0, 0 };
const Vec3 DIRECTION_LEFT   = { -1, 0, 0 };
const Vec3 DIRECTION_TOP    = { 0, 1, 0 };
const Vec3 DIRECTION_BOTTOM = { 0, -1, 0 };
const Vec3 DIRECTION_FAR    = { 0, 0, -1 };
const Vec3 DIRECTION_NEAR   = { 0, 0, 1 };

struct TextureSystem;
struct EngineContext;

// Geometry.

struct Vertex
{
    Vec3 pos;
    Vec3 normal;
    Vec2 tex_coord;
};

struct GeometryView
{
    static constexpr u32 MASK_IS_INDEXED = 0b1;
    static constexpr u32 MASK_IDX_STRIDE = 0b1110;
    static constexpr u32 MASK_COUNT = ~u32(0) & ~(MASK_IDX_STRIDE | MASK_IS_INDEXED);

    static GeometryView create(sz vertices_offset, sz vertices_count, sz size_bytes);
    static GeometryView create_indexed(
        sz indices_offset, sz indices_count, sz vertices_offset,
        sz size_bytes, IndexStride idx_stride = INDEX_STRIDE_CHAR
    );
    
    // Uses u32 as internal storage type to pack data better.
    // [count=31..5;idx_stride=4..1;is_indexed=0..0]
	BitInt<u32> bits;
    u32 indices_offset;
    u32 vertices_offset;
	// Either vertex_byte_size of index_byte_size, depending on 'is_indexed'
	u32 size_bytes;

	u32 count() const { return (u32)this->bits.get_mask(MASK_COUNT); }
	bool is_indexed() const { return (bool)this->bits.get_mask(MASK_IS_INDEXED); }
	u8 idx_stride() { return (u8)this->bits.get_mask(MASK_IDX_STRIDE); }

	void set_count(u32 new_count)
	{
	    this->bits.set_mask(new_count, MASK_COUNT);
	}
	void set_is_indexed(bool new_indexed)
	{
	    this->bits.set_mask((u32)new_indexed, MASK_IS_INDEXED);
	}
	void set_idx_stride(u8 new_idx_stride)
	{
	    this->bits.set_mask((u32)new_idx_stride, MASK_IDX_STRIDE);
	}
};

// IndexBuffer.
// Temporary storage for the index data.
// Allocated while gltf model is parsed,
// deallocated after indices were appended into gpu-side buffer.

struct IndexBuffer
{
	static constexpr sz DEFAULT_CAPACITY = 2048;

	Slice<u8> data;
	IndexStride stride;

	static IndexBuffer create(Slice<u8> data, IndexStride stride);

	sz indices_count() const { return data.count / stride; }
	sz byte_size() const { return data.count; }
};

struct ElementBuffer
{
	VulkanBufferCpu staging_buff;
	VulkanBuffer main_buff;
	// TODO: maybe return
	// Mutex mutex;

	void init(sz init_capacity);
    GeometryView append_geometry(Slice<Vertex> vertices);
    GeometryView append_geometry_indexed(Slice<Vertex> vertices, const IndexBuffer& idx_buff);
    void cmd_copy_data_to_gpu(VulkanCmdBuffer cmd_buff, sz offset = 0, sz size = 0);
    void cmd_copy_data_to_gpu_from_view(VulkanCmdBuffer cmd_buff, GeometryView geometry);
    void cmd_bind(VulkanCmdBuffer cmd_buff, GeometryView view);
    void destroy(VulkanContext* ctx);

	sz size() const { return this->staging_buff.size; }
	sz capacity() const { return this->staging_buff.capacity; }
};

// Camera.

struct Camera
{
    static constexpr f32 DEFAULT_SPEED = 32.0f;
    static constexpr f32 ZOOM_SPEED = deg_to_rad(1.0f);
    static constexpr f32 DEFAULT_ZOOM = deg_to_rad(45.0f);
    static constexpr f32 ZOOM_MIN = deg_to_rad(0.1f);
    static constexpr f32 ZOOM_MAX = deg_to_rad(300.0f);

	Vec3 pos;
	Vec3 target;
	Vec3 right;
	Vec3 up;
	f32 yaw;
	f32 pitch;
	f32 speed;
	// TODO: make it in radians
	f32 zoom;
	f32 mouse_sensivity;
	bool dirty;

    void init(Vec3 pos, Vec3 target, f32 speed = DEFAULT_SPEED, f32 zoom = DEFAULT_ZOOM);
    Mat4 look_at();
    void handle_mouse_move(f32 delta_x, f32 delta_y);
    void handle_key_down(u32 key, Nanoseconds delta_time_ns);
    void handle_mouse_wheel(bool delta);
    void recalc_state(f32 delta_x, f32 delta_y);
};

// Entity.

// Shader binding order.
alias EntityTextureBinding = EntityTextureKind;

struct EntityTransforms
{
	union
	{
		struct
		{
			Quat rotation;
			Vec3 scale;
			Vec3 translation;
		};
		Mat4 matrix;
	};

	EntityTransforms() = default;
	EntityTransforms(const EntityTransforms& rhs)
	{
		rg::mem_copy(this, &rhs, sizeof(*this));
	}
	EntityTransforms& operator=(const EntityTransforms& rhs)
	{
		ASSERT(this != &rhs);
		rg::mem_copy(this, &rhs, sizeof(*this));
		return *this;
	}
};

// Not used in Entity, for space efficiency reasons.
// Used in entity initialization for the ease of caller.
struct EntityTransformsTagged
{
	union
	{
		struct
		{
			Vec3 translation;
			Quat rotation;
			Vec3 scale;
		};
		Mat4 matrix;
	};
	Vec3 velocity;
	Vec3 rotation_update;
	bool has_matrix;

	EntityTransformsTagged() = default;
	EntityTransformsTagged(const EntityTransformsTagged& rhs)
	{
		rg::mem_copy(this, &rhs, sizeof(*this));
	}
	EntityTransformsTagged& operator=(const EntityTransformsTagged& rhs)
	{
		ASSERT(this != &rhs);
		rg::mem_copy(this, &rhs, sizeof(*this));
		return *this;
	}

	static EntityTransformsTagged create(Vec3 translation, Vec3 velocity, Vec3 scale, Quat rotation, Vec3 rotation_update);
	static EntityTransformsTagged create_matrix(const Mat4& matrix);
};

struct EntitySystem;

struct Entity
{
	GeometryView geometry;
	EntityShaderState shader_state;
	// EntityTransforms transforms;

	u32 position_idx;
	u32 velocity_idx;
	u32 scale_idx;
	u32 rotation_idx;

	TextureIdx diffuse_tex;
	TextureIdx normal_tex;
	TextureIdx occlussion_tex;
	TextureIdx emissive_tex;
	TextureIdx metallic_roughness_tex;

	static Entity create(
		EngineContext* engine_ctx,
		GeometryView geometry,
		Slice<TextureCreateConfig> tex_configs,
		const EntityTransformsTagged& transforms
	);
	void init(
		EngineContext* engine_ctx,
		GeometryView geometry,
		Slice<TextureCreateConfig> tex_configs,
		const EntityTransformsTagged& transforms
	);
	Mat4 get_model(const EntitySystem& en_sys, Slice<TransformType> transform_order);
	void write_updates_for_shader(TextureSystem* tex_sys, FArray<DescriptorUpdateInfo, MAX_DESCRIPTOR_BINDING_COUNT>* out_update_infos);
	void destroy(EngineContext* engine_ctx, VulkanShader* shader);

	Slice<TextureIdx> tex_indices_as_arr()
	{
		return { &diffuse_tex, 5 };
	}
};

struct EntitySystem
{
	static constexpr sz DEFAULT_CAPACITY = 128;
	static constexpr sz COMPONENTS_COUNT = 4;
	
	DArrayUnmanaged<Vec3> positions;
	DArrayUnmanaged<Vec3> velocities;
	DArrayUnmanaged<Quat> rotations;
	DArrayUnmanaged<Vec3> rotation_updates;
	DArrayUnmanaged<Vec3> scales;
	Allocator* alloc;

	// Capacity in elements.
	void init(Allocator* alloc, sz init_capacity = DEFAULT_CAPACITY);
	void apply_velocities();
	void apply_rotations();
	// Capacity in elements.
	void realloc(sz need_capacity);
	void destroy();

	u32 add_position_and_velocity(Vec3 pos, Vec3 vel);
	u32 add_rotation_and_update(Quat rotation, Vec3 update);
	u32 add_scale(Vec3 scale);

	Vec3 get_position(u32 idx) const;
	Vec3 get_velocity(u32 idx) const;
	Vec3 get_scale(u32 idx) const;
	Quat get_rotation(u32 idx) const;
};

// Renderer.

struct Renderer
{
    static constexpr sz INIT_ENTITY_COUNT            = 128;
    static constexpr sz INIT_ELEM_BUFF_CAPACITY      = 128 * MB;
    static constexpr sz MAX_BACKGROUND_TASKS         = 64;
    static constexpr u8 MASK_EXIT_SCHEDULED          = 0b1;
    static constexpr u8 MASK_RESIZE_SCHEDULED        = 0b10;

    // NOTE: maybe slot array?
	DArray<Entity> entities;
	Mat4 view;
	Mat4 proj;
	ElementBuffer elem_buff;
	Camera camera;
	Clock frame_clock;
	VkViewport viewport;
	VkRect2D scissors;
	ColorRGBA clear_color;
	// [exit_scheduled=1;resize_scheduled=0]
	BitInt<u8> bits;

    void init(VkExtent2D init_area, ColorRGBA init_clear_color);
    SwapchainPresentResult begin_frame();
    void draw_frame();
    bool end_frame();
    void update_view_projection();
    void destroy();

    void start_clock()
    {
		this->frame_clock.start();
    }
	void set_clear_color(ColorRGBA clear_color = COLOR_BLACK_RGBA)
	{
		this->clear_color = clear_color;
	}
	void update_viewport_scissors(VkExtent2D area)
	{
		this->viewport.width = (f32)area.width;
		this->viewport.height = (f32)area.height;
		this->scissors.extent = area;
	}
	VkExtent2D get_extent()
	{
		return this->scissors.extent;
	}
	f32 aspect_ratio()
	{
		VkExtent2D extent = this->get_extent();
		return (f32)extent.width / extent.height;
	}
	bool should_close()
	{
		return this->exit_scheduled();
	}
	void set_view(const Mat4& view)
	{
		this->view = view;
	}
	void set_proj(const Mat4& proj)
	{
		this->proj = proj;
	}

    // Bitfields.
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
