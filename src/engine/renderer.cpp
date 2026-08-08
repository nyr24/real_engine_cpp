#include "core/math.hpp"
#include "glfw/glfw3.h"
#include "engine/renderer.hpp"
#include "engine/entry.hpp"
#include "engine/texture.hpp"

namespace rg
{

// Geometry.

bool operator==(const GeometryView& a, const GeometryView& b)
{
	if (a.is_indexed() != b.is_indexed()) return false;

	if (a.is_indexed())
	{
		return a.count() == b.count()
			&& a.indices_offset == b.indices_offset
			&& a.vertices_offset == b.vertices_offset;
	}

	return a.count() == b.count()
		&& a.vertices_offset == b.vertices_offset;
}

GeometryView GeometryView::create_indexed(sz indices_offset, sz indices_count, sz vertices_offset, sz size_bytes, IndexStride idx_stride)
{
	GeometryView view = {
    	.indices_offset = (u32)indices_offset,
    	.vertices_offset = (u32)vertices_offset,
		.size_bytes = (u32)size_bytes,
	};
    view.set_count((u32)indices_count);
	view.set_idx_stride(idx_stride);
	view.set_is_indexed(true);
	return view;
}

GeometryView GeometryView::create(sz vertices_offset, sz vertices_count, sz size_bytes)
{
	GeometryView view = {
    	.vertices_offset = (u32)vertices_offset,
		.size_bytes = (u32)size_bytes,
	};
	view.set_is_indexed(false);
    view.set_count((u32)vertices_count);
	return view;
}

// Index buffer.

IndexBuffer IndexBuffer::create(Slice<u8> data, IndexStride stride)
{
	IndexBuffer idx_buff;
	idx_buff.data = data;
	idx_buff.stride = stride;
	return idx_buff;
}

// Element buffer.

intern VkIndexType convert_stride_to_vk_index(IndexStride stride);
intern IndexStride convert_vk_index_to_stride(VkIndexType vk_index);

void ElementBuffer::init(sz init_capacity)
{
	EngineContext* engine_ctx = get_engine_context();
	// this->mutex.init();

	this->staging_buff.init(
		&engine_ctx->vk_ctx,
		init_capacity,
		{
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.sharing_mode = VK_SHARING_MODE_EXCLUSIVE,
			.mem_props = (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
		}
	);

	this->main_buff.init(
		&engine_ctx->vk_ctx,
		init_capacity,
		{
			.usage = (VkBufferUsageFlagBits)(
				VK_BUFFER_USAGE_TRANSFER_DST_BIT
				| VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
				| VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
			.sharing_mode = VK_SHARING_MODE_EXCLUSIVE,
			.mem_props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		}
	);
}

GeometryView ElementBuffer::append_geometry_indexed(Slice<Vertex> vertices, const IndexBuffer& idx_buff)
{
	sz vertices_size = vertices.count * sizeof(Vertex);
	sz indices_size  = idx_buff.byte_size();
	sz capacity_sum  = vertices_size + indices_size;

	Slice<u8> vertices_bytes = vertices.to_byte_slice();
	Array<Slice<u8>, 2> data_to_append = { vertices_bytes, idx_buff.data };
	EngineContext* engine_ctx = get_engine_context();

	engine_ctx->mutex.lock();
	BufferChunkView chunk_view = this->staging_buff.append_data(data_to_append.slice());
	engine_ctx->mutex.unlock();

	if (this->staging_buff.capacity > this->main_buff.capacity)
	{
		this->main_buff.realloc(this->staging_buff.capacity);
	}

	GeometryView view_indexed = GeometryView::create_indexed(
		chunk_view.offset + vertices_size,
		idx_buff.indices_count(),
		chunk_view.offset,
		capacity_sum,
		idx_buff.stride
	);

	return view_indexed;
}

GeometryView ElementBuffer::append_geometry(Slice<Vertex> vertices)
{
	Slice<u8> vertices_as_bytes = vertices.to_byte_slice();
	Array<Slice<u8>, 1> data_to_append = { vertices_as_bytes };
	BufferChunkView chunk_view = this->staging_buff.append_data(data_to_append.slice());

	if (this->staging_buff.capacity > this->main_buff.capacity)
	{
		this->main_buff.realloc(this->staging_buff.capacity);
	}

	GeometryView view = GeometryView::create(chunk_view.offset, vertices.count, chunk_view.size);
	return view;
}

void ElementBuffer::cmd_copy_data_to_gpu(VulkanCmdBuffer cmd_buff, sz offset, sz size)
{
	size = size == 0 ? this->size() : size;
	VkBufferCopy copy_region = { .srcOffset = (usz)offset, .dstOffset = (usz)offset, .size = (usz)size };
	vkCmdCopyBuffer(cmd_buff.handle, this->staging_buff.handle, this->main_buff.handle, 1, &copy_region);
}

void ElementBuffer::cmd_copy_data_to_gpu_from_view(VulkanCmdBuffer cmd_buff, GeometryView geometry)
{
	sz offset = geometry.vertices_offset;
	sz size   = geometry.size_bytes;
	ASSERT_MSG(offset + size <= this->capacity(), "Shouldn't exceed capacity");
	VkBufferCopy copy_region = { .srcOffset = (usz)offset, .dstOffset = (usz)offset, .size = (usz)size };
	vkCmdCopyBuffer(cmd_buff.handle, this->staging_buff.handle, this->main_buff.handle, 1, &copy_region);
}

void ElementBuffer::cmd_bind(VulkanCmdBuffer cmd_buff, GeometryView view)
{
	Array<VkDeviceSize, 1> offsets = { 0 };
	vkCmdBindVertexBuffers(cmd_buff.handle, 0, 1, &this->main_buff.handle, offsets.data);

	if (view.is_indexed())
	{
		vkCmdBindIndexBuffer(
			cmd_buff.handle,
			this->main_buff.handle,
			view.indices_offset,
			convert_stride_to_vk_index((IndexStride)view.idx_stride())
		);
	}
}

void ElementBuffer::destroy(VulkanContext* ctx)
{
	this->staging_buff.destroy(ctx);
	this->main_buff.destroy(ctx);
	// this->mutex.destroy();
}

intern VkIndexType convert_stride_to_vk_index(IndexStride stride)
{
	switch (stride)
	{
		case INDEX_STRIDE_CHAR: return VK_INDEX_TYPE_UINT8;
		case INDEX_STRIDE_SHORT: return VK_INDEX_TYPE_UINT16;
		case INDEX_STRIDE_INT: return VK_INDEX_TYPE_UINT32;
		default: UNREACHABLE("Unknown byte stride value: %u", stride);
	}
}

intern IndexStride convert_vk_index_to_stride(VkIndexType vk_index)
{
	switch (vk_index)
	{
		case VK_INDEX_TYPE_UINT8: return INDEX_STRIDE_CHAR;
		case VK_INDEX_TYPE_UINT16: return INDEX_STRIDE_SHORT;
		case VK_INDEX_TYPE_UINT32: return INDEX_STRIDE_INT;
		default: UNREACHABLE("Unknown VkIndexType %u", vk_index);
	}
}

// Camera.

void Camera::init(Vec3 pos, Vec3 target, f32 speed, f32 zoom)
{
	this->pos = pos;
	this->target = target;
	this->right = DIRECTION_RIGHT;
	this->up = DIRECTION_TOP;
	this->yaw = rg::deg_to_rad(-90.0f);
	this->pitch = 0;
	this->speed = speed;
	this->zoom = zoom;
	this->dirty = true;
	this->mouse_sensivity = MOUSE_SENSIVITY_MEDIUM;
}

Mat4 Camera::look_at()
{
// Define if debug info needed.
#if 0
	// NOTE: TEMP
	LOG_DEBUG("Camera staying at: ");
	this->pos.print(LogLevel::DEBUG);
	LOG_DEBUG("Camera looking at:");
	this->target.print(LogLevel::DEBUG);
	LOG_DEBUG("Camera up is:");
	this->up.print(LogLevel::DEBUG);
#endif

	return Mat4::look_at(this->pos, this->pos + this->target, this->up);
}

void Camera::handle_mouse_move(f32 delta_x, f32 delta_y)
{
	this->yaw += delta_x * this->mouse_sensivity;
	this->pitch = rg::clamp(
		this->pitch + (delta_y * this->mouse_sensivity),
		rg::deg_to_rad(-89.0f),
		rg::deg_to_rad(89.0f)
	);
	this->recalc_state(delta_x, delta_y);
	this->dirty = true;
}

void Camera::handle_key_down(u32 key, Nanoseconds delta_time_ns)
{
	intern constexpr f32 DELTA_TIME_MULTIPLIER = (f32)1 / 100'000'000'0;
	f32 velocity = this->speed * delta_time_ns * DELTA_TIME_MULTIPLIER;

	switch (key)
	{
		case GLFW_KEY_W:
			this->pos += this->target * velocity;
			break;
		case GLFW_KEY_S:
			this->pos -= this->target * velocity;
			break;
		case GLFW_KEY_D:
			this->pos += this->right * velocity;
			break;
		case GLFW_KEY_A:
			this->pos -= this->right * velocity;
			break;
		default:
			return;
	}
	this->dirty = true;
}

// THINK: wheel update don't require look_at update
// so maybe change 'dirty' to bitstruct, so that we know
// which matrix updates are triggerred by camera updates (view, proj, both)
void Camera::handle_mouse_wheel(bool delta)
{
	// NOTE: TEMP
	LOG_DEBUG("wheel event: delta: %u", (u32)delta);
	if (delta)
	{
		this->zoom += Camera::ZOOM_SPEED;
	}
	else
	{
		this->zoom -= Camera::ZOOM_SPEED;
	}
	this->zoom = rg::clamp(this->zoom, Camera::ZOOM_MIN, Camera::ZOOM_MAX);
	this->dirty = true;
}

void Camera::recalc_state(f32 delta_x, f32 delta_y)
{
	this->target.x = rg::cos(this->yaw) * rg::cos(this->pitch);
	this->target.y = rg::sin(this->pitch);
	this->target.z = rg::sin(this->yaw) * rg::cos(this->pitch);
	this->target.normalize_inplace();
	this->right = vec_cross(this->target, DIRECTION_TOP);
	this->right.normalize_inplace();
	this->up = vec_cross(this->right, this->target);
	this->up.normalize_inplace();
}

// Entity.

Entity Entity::create(
	EngineContext* engine_ctx,
	GeometryView geometry,
	Slice<TextureCreateConfig> tex_configs,
	const EntityTransformsTagged& transforms
)
{
	Entity res;
	res.init(engine_ctx, geometry, tex_configs, transforms);
	return res;
}

void Entity::init(
	EngineContext* engine_ctx,
	GeometryView geometry,
	Slice<TextureCreateConfig> tex_configs,
	const EntityTransformsTagged& transforms
)
{
	VulkanShader* shader = engine_ctx->vk_ctx.get_curr_shader();
	this->geometry = geometry;
	this->shader_state = shader->allocate_entity_resources(engine_ctx);

	if (transforms.has_matrix)
	{
		this->transforms.matrix = transforms.matrix;
		this->shader_state.set_has_matrix(true);
	}
	else
	{
		this->transforms.rotation = transforms.rotation;
		this->transforms.scale = transforms.scale;
		this->transforms.translation = transforms.translation;
		this->shader_state.set_has_matrix(false);
	}

	// Process textures.
	Slice<TextureIdx> tex_indices = this->tex_indices_as_arr();
	for (sz i = 0; i < MAX_ENTITY_TEX_COUNT; ++i)
	{
		tex_indices[i] = TEX_INDEX_INVALID;
	}

	for (auto& conf : tex_configs)
	{
		switch (conf.kind)
		{
			case EntityTextureKind::DIFFUSE:
				this->diffuse_tex = conf.idx;
				break;
			case EntityTextureKind::NORMAL:
				this->normal_tex = conf.idx;
				break;
			case EntityTextureKind::OCCLUSSION:
				this->occlussion_tex = conf.idx;
				break;
			case EntityTextureKind::EMISSIVE:
				this->emissive_tex = conf.idx;
				break;
			case EntityTextureKind::METALLIC_ROUGHNESS:
				this->metallic_roughness_tex = conf.idx;
				break;
			default: UNREACHABLE("Unknown entity texture kind: %d", conf.kind);
		}
	}
}

Mat4 Entity::get_model(Slice<TransformType> transform_order)
{
	if (this->shader_state.has_matrix())
	{
		return this->transforms.matrix;
	}
	else
	{
		Mat4 model = Mat4::identity();
		for (TransformType order : transform_order)
		{
			switch (order)
			{
				case TransformType::SCALE:
					model.scale_inplace(this->transforms.scale);
					break;
				case TransformType::TRANSLATION:
					model.translate_inplace(this->transforms.translation);
					break;
				case TransformType::ROTATION:
					model *= this->transforms.rotation.to_matrix();
					break;
				default: UNREACHABLE("Unknown transform type");
			}
		}
		return model;
	}
}

void Entity::write_updates_for_shader(
	TextureSystem* tex_sys,
	FArray<DescriptorUpdateInfo, MAX_DESCRIPTOR_BINDING_COUNT>* out_update_infos
)
{
	Slice<TextureIdx> indices = this->tex_indices_as_arr();

	for (sz i = 0; i < indices.count; ++i)
	{
		TextureIdx tex_idx = indices[i];
		if (tex_idx == TEX_INDEX_INVALID) continue;

		Texture* tex = tex_sys->get_texture_by_idx(tex_idx);
		ASSERT_MSG(tex, "Texture shouldn't be null");
		ASSERT_MSG(tex->load_state == TextureLoadState::LOADED_GPU, "Texture should be uploaded to gpu");

		out_update_infos->push(
			{
				.set_index = 0,
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.image_info = tex->to_vk_descriptor_info(),
			}
		);
	}
}

void Entity::destroy(EngineContext* engine_ctx, VulkanShader* shader)
{
	shader->destroy_entity_resources(&engine_ctx->vk_ctx, this->shader_state);
	Slice<TextureIdx> indices = this->tex_indices_as_arr();
	for (auto idx : indices)
	{
		if (idx == TEX_INDEX_INVALID) continue;
		engine_ctx->tex_sys.remove_texture(idx);
	}
}

EntityTransformsTagged EntityTransformsTagged::create(Vec3 translation, Vec3 scale, Quat rotation)
{
	EntityTransformsTagged res;
	res.has_matrix = false;
	res.rotation = rotation;
	res.scale = scale;
	res.translation = translation;
	return res;
}

EntityTransformsTagged EntityTransformsTagged::create_matrix(const Mat4& matrix)
{
	EntityTransformsTagged res;
	res.has_matrix = true;
	res.matrix = matrix;
	return res;
}

// Renderer.

// NOTE: TEMP
const u32 MODEL_LOAD_KEY = GLFW_KEY_M;

intern void init_event_handlers(Renderer* self, EventSystem* ev_sys);;
intern bool handle_mouse_move(EventContext ctx, void* listener);
intern bool handle_key_down_renderer(EventContext ctx, void* listener);
intern bool handle_key_down_camera(EventContext ctx, void* listener);
intern bool handle_mouse_wheel(EventContext ctx, void* listener);
intern bool handle_model_load(EventContext ctx, void* listener);
intern void cmd_begin_rendering(VulkanCmdBuffer cmd_buff, VkRenderingInfo* rendering_info);
intern void cmd_end_rendering(VulkanCmdBuffer cmd_buff);
intern void cmd_draw(VulkanCmdBuffer cmd_buff, GeometryView view);
intern void cmd_set_viewport_scissors(VulkanCmdBuffer cmd_buff, Renderer* renderer);
intern void cmd_push_mvp(VulkanCmdBuffer cmd_buff, VulkanPipeline* pipeline, const Mat4& model, const Mat4& view, const Mat4& proj);
intern void cmd_set_viewport_scissors(VulkanCmdBuffer cmd_buff, Renderer* renderer);
intern void begin_rendering(EngineContext* ctx, VulkanCmdBuffer cmd_buffer);
intern void end_rendering(VulkanContext* ctx, VulkanCmdBuffer cmd_buffer);
intern void add_entity(GeometryView geometry, Slice<TextureCreateConfig> tex_configs, EntityTransformsTagged transforms);
intern void bind_entity(Entity* entity, VulkanShader* shader, VulkanPipeline* pipeline, VulkanCmdBuffer cmd_buff, sz curr_frame);
intern void update_entity(Entity* entity, VulkanShader* shader, sz curr_frame);
intern void perform_startup_load(Renderer* renderer);
intern GltfSampler get_default_gltf_sampler();
intern IndexBuffer get_cube_indices(IndexStride stride = INDEX_STRIDE_INT);
intern Array<Vertex, CUBE_VERT_COUNT> get_cube_vertices();

void Renderer::init(VkExtent2D init_area, ColorRGBA init_clear_color)
{
	this->clear_color = init_clear_color;
	this->viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = (f32)init_area.width,
		.height = (f32)init_area.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	this->scissors = { .offset = { 0, 0 }, .extent = init_area };

	Context* ctx = get_context();
	EngineContext* engine_ctx = get_engine_context();

	this->elem_buff.init(INIT_ELEM_BUFF_CAPACITY);

	this->camera.init({ 0, 0.5, 4 }, DIRECTION_FAR);
	this->update_view_perspective();

	init_event_handlers(this, &engine_ctx->event_sys);

	this->entities.init_capacity(ctx->allocator, INIT_ENTITY_COUNT);

	perform_startup_load(this);
}

intern void perform_startup_load(Renderer* renderer)
{
	Array<Vertex, CUBE_VERT_COUNT> cube_vertices = get_cube_vertices();
	IndexBuffer cube_indices = get_cube_indices();
	GeometryView cube_geometry = renderer->elem_buff.append_geometry_indexed(cube_vertices.slice(), cube_indices);

	GltfSampler default_sampler = get_default_gltf_sampler();

	EngineContext* engine_ctx = get_engine_context();
	VulkanContext* vk_ctx = &engine_ctx->vk_ctx;
	ThreadArena* persist_alloc = engine_ctx->persist_allocator;

	constexpr sz MAX_LOAD_COUNT = 16;

	FArray<Array<StrView, 2>, MAX_LOAD_COUNT> paths_parts;
	paths_parts.push({ TEXTURES_PATH, "soil.jpg" });
	paths_parts.push({ TEXTURES_PATH, "sand.jpg" });
	paths_parts.push({ TEXTURES_PATH, "metal.jpg" });
	paths_parts.push({ TEXTURES_PATH, "water.jpg" });
	paths_parts.push({ TEXTURES_PATH, "stones.jpg" });
	paths_parts.push({ TEXTURES_PATH, "grass.jpg" });

	// TODO: not effective, we're loading textures 1 by 1, creating threads for that.
	// We should load them in 1 batch.
	FArray<TextureCreateConfig, MAX_LOAD_COUNT> tex_configs;
	tex_configs.push({ Path::create(persist_alloc, paths_parts[0].slice(), true), default_sampler, TEX_INDEX_INVALID, EntityTextureKind::DIFFUSE });
	tex_configs.push({ Path::create(persist_alloc, paths_parts[1].slice(), true), default_sampler, TEX_INDEX_INVALID, EntityTextureKind::DIFFUSE });
	tex_configs.push({ Path::create(persist_alloc, paths_parts[2].slice(), true), default_sampler, TEX_INDEX_INVALID, EntityTextureKind::DIFFUSE });
	tex_configs.push({ Path::create(persist_alloc, paths_parts[3].slice(), true), default_sampler, TEX_INDEX_INVALID, EntityTextureKind::DIFFUSE });
	tex_configs.push({ Path::create(persist_alloc, paths_parts[4].slice(), true), default_sampler, TEX_INDEX_INVALID, EntityTextureKind::DIFFUSE });
	tex_configs.push({ Path::create(persist_alloc, paths_parts[5].slice(), true), default_sampler, TEX_INDEX_INVALID, EntityTextureKind::DIFFUSE });

	// NOTE: TEMP
	for (const auto& conf : tex_configs)
	{
		LOG_TRACE("Will load texture from path: " FMT_PLACEHOLDER_LEN, FMT_DSTRING_VAL(conf.path));
	}

	FrameData* frame = vk_ctx->get_curr_frame_data();

	cmd_buffer_begin_recording_many(frame->transfer_cmd_buffs.slice(), frame->transfer_cmd_buff_states.slice());

	{
		auto* talloc = get_temp_allocator();
		TEMP_ALLOC_SCOPE(talloc);

		engine_ctx->tex_sys.load_new_textures_cpu_and_transfer_to_gpu(tex_configs.slice(), frame->transfer_cmd_buffs.slice());

		const Vec3 SCALE = { 1, 1, 1 };
		const Quat ROTATION = Quat::identity();
		const Quat ROTATION_2 = Quat::from_euler(30, 45, 0);
		const Quat ROTATION_3 = Quat::from_euler(90, 180, 0);
		const Quat ROTATION_4 = Quat::from_euler(270, -35, 0);
		const Vec3 TRANSLATE = { 0, 0, -5 };

		add_entity(cube_geometry, { &tex_configs[0], 1 }, EntityTransformsTagged::create(TRANSLATE, SCALE, ROTATION));
		add_entity(cube_geometry, { &tex_configs[1], 1 }, EntityTransformsTagged::create({ 2, 3.5, -3 }, { 2, 2, 2 }, ROTATION_2));
		add_entity(cube_geometry, { &tex_configs[2], 1 }, EntityTransformsTagged::create({ 0.7, -0.1, -3 }, { 2, 0.5, 1 }, ROTATION_3));
		add_entity(cube_geometry, { &tex_configs[3], 1 }, EntityTransformsTagged::create({ -0.1, 0.8, 2 }, { 3, 0.5, 0.5 }, ROTATION_4));
		add_entity(cube_geometry, { &tex_configs[4], 1 }, EntityTransformsTagged::create({ 0.4, 0.4, -1 }, SCALE, ROTATION));
		add_entity(cube_geometry, { &tex_configs[5], 1 }, EntityTransformsTagged::create({ 0.7, 0.7, 1.0 }, { 4, 2, 2 }, ROTATION_4));

		// NOTE: TEMP -->
		// Path[] paths    = f_arena.alloc_array(Path, 1);
		// Path model_path = path::new(f_arena, MODELS_PATH +++ "scifi_helmet/SciFiHelmet.gltf")!!;
		// paths[0] = model_path;
		// LoadModelsTaskData task_data = { .paths = paths, .app_ctx = renderer.app_ctx, };
		// BackgroundTask task          = { .kind = LOAD_MODELS, .payload = { .load_models = task_data }, };
		// // TODO: this is not loading anything.
		// renderer_process_background_load_models_task(renderer, task);
		// // renderer_process_background_load_models_task_temp(renderer, task);
		// NOTE: TEMP <--

		cmd_buffer_end_submit_reset_many(
			frame->transfer_cmd_buffs.slice(),
			frame->transfer_cmd_buff_states.slice(),
			vk_ctx,
			vk_ctx->dev.transfer_queue,
			true
		);
	};
}

SwapchainPresentResult Renderer::begin_frame()
{
	// renderer_process_background_tasks(&engine_ctx->vk_ctx.renderer);

	EngineContext* engine_ctx = get_engine_context();
	VulkanContext* vk_ctx = &engine_ctx->vk_ctx;
	vk_ctx->swapchain.recreate_if_needed(vk_ctx);
	FrameData* frame = vk_ctx->get_curr_frame_data();

	return vk_ctx->swapchain.acquire_next_image_index(vk_ctx, frame->image_avail_sem);
}

void Renderer::draw_frame()
{
	EngineContext* engine_ctx = get_engine_context();
	VulkanContext* vk_ctx    = &engine_ctx->vk_ctx;
	FrameData* frame         = vk_ctx->get_curr_frame_data();
	ElementBuffer* elem_buff = &this->elem_buff;
	VulkanShader* shader     = vk_ctx->get_curr_shader();
	VulkanPipeline* pipeline = shader->get_curr_pipeline();
	sz curr_frame            = vk_ctx->curr_frame;

	// NOTE: temp 0 buffer, maybe should be different.
	VulkanCmdBuffer* transfer_cmd_buff    = &frame->transfer_cmd_buffs[0];
	CmdBufferState* transfer_cmd_buff_state = &frame->transfer_cmd_buff_states[0];
	VulkanCmdBuffer* graphics_cmd_buff    = &frame->graphics_cmd_buffs[0];
	CmdBufferState* graphics_cmd_buff_state = &frame->graphics_cmd_buff_states[0];

	// --- Step: transfer resources
	transfer_cmd_buff->begin_recording(transfer_cmd_buff_state);

	if (this->resize_scheduled() || this->camera.dirty)
	{
		// NOTE: (HACK) only start receiving move events when first resize happens
		// because it will have big mouse delta to go from init hardcoded window coords
		// to actual window coords
		// if (!engine_ctx->input_sys.accept_move_events && this->resize_scheduled())
		// {
		// 	engine_ctx->input_sys.accept_move_events = true;
		// }
		this->update_view_perspective();
	}

	// TODO: remove copying data every frame, use task system.
	elem_buff->cmd_copy_data_to_gpu(*transfer_cmd_buff);

	transfer_cmd_buff->end_recording(transfer_cmd_buff_state);

	transfer_cmd_buff->submit(
		transfer_cmd_buff_state,
		vk_ctx,
		vk_ctx->dev.transfer_queue,
		{ &frame->transfer_end_sem, 1 }
	);

	// --- Step: bind resources
	graphics_cmd_buff->begin_recording(graphics_cmd_buff_state);

	pipeline->cmd_bind(*graphics_cmd_buff);
	cmd_set_viewport_scissors(*graphics_cmd_buff, this);

	// --- Step: draw into the render target
	begin_rendering(engine_ctx, *graphics_cmd_buff);

	GeometryView curr_geometry      = { };
	GeometryView last_bind_geometry = { };

	for (auto& entity : this->entities)
	{
		curr_geometry = entity.geometry;

		if (curr_geometry != last_bind_geometry)
		{
			elem_buff->cmd_bind(*graphics_cmd_buff, curr_geometry);
			last_bind_geometry = curr_geometry;
		}

		persist Array<TransformType, 3> transform_order = {
			TransformType::SCALE,
			TransformType::ROTATION,
			TransformType::TRANSLATION
		};

		cmd_push_mvp(
			*graphics_cmd_buff,
			pipeline,
			entity.get_model(transform_order.slice()),
			this->view,
			this->proj
		);

		update_entity(&entity, shader, curr_frame);
		bind_entity(&entity, shader, pipeline, *graphics_cmd_buff, curr_frame);
		cmd_draw(*graphics_cmd_buff, curr_geometry);
	}

	end_rendering(vk_ctx, *graphics_cmd_buff);
	graphics_cmd_buff->end_recording(graphics_cmd_buff_state);

	// --- Step submit graphics buffer
	Array<VulkanSemaphore, 2> wait_semaphores = { frame->image_avail_sem, frame->transfer_end_sem };
	Array<VkPipelineStageFlags, 2> wait_dest_stage_masks = {
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
	};
	Array<VulkanSemaphore, 1> signal_semaphores = { frame->render_end_sem };

	graphics_cmd_buff->submit(
		graphics_cmd_buff_state,
		vk_ctx,
		vk_ctx->dev.graphics_queue,
		signal_semaphores.slice(),
		wait_semaphores.slice(),
		wait_dest_stage_masks.slice(),
		Maybe<VulkanFence>{ frame->render_end_fence }
	);

	vk_ctx->swapchain.present(vk_ctx, vk_ctx->dev.present_queue, frame->render_end_sem);
}

bool Renderer::end_frame()
{
	EngineContext* engine_ctx = get_engine_context();
	VulkanContext* vk_ctx = &engine_ctx->vk_ctx;
	FrameData* frame      = vk_ctx->get_curr_frame_data();
	if (!frame->render_end_fence.wait(vk_ctx)) return false;
	frame->render_end_fence.reset(vk_ctx);

	VulkanCmdBuffer* transfer_cmd_buff    = &frame->transfer_cmd_buffs[0];
	CmdBufferState* transfer_cmd_buff_state = &frame->transfer_cmd_buff_states[0];
	VulkanCmdBuffer* graphics_cmd_buff    = &frame->graphics_cmd_buffs[0];
	CmdBufferState* graphics_cmd_buff_state = &frame->graphics_cmd_buff_states[0];

	transfer_cmd_buff->reset(transfer_cmd_buff_state);
	graphics_cmd_buff->reset(graphics_cmd_buff_state);

	engine_ctx->renderer.frame_clock.update();
	engine_ctx->input_sys.update();
	vk_ctx->advance_frame();
	return !engine_ctx->renderer.exit_scheduled();
}

intern void add_entity(GeometryView geometry, Slice<TextureCreateConfig> tex_configs, EntityTransformsTagged transforms)
{
	EngineContext* engine_ctx = get_engine_context();
	engine_ctx->renderer.entities.push(Entity::create(engine_ctx, geometry, tex_configs, transforms));
}

intern void update_entity(Entity* entity, VulkanShader* shader, sz curr_frame)
{
	EngineContext* engine_ctx = get_engine_context();
	VulkanContext* vk_ctx = &engine_ctx->vk_ctx;
	FArray<DescriptorUpdateInfo, MAX_DESCRIPTOR_BINDING_COUNT> update_infos;
	entity->write_updates_for_shader(&engine_ctx->tex_sys, &update_infos);
	if (update_infos.count == 0)
	{
		LOG_WARN("Update infos count is zero, this is not usual");
		return;
	}
	shader->update_entity_resources(vk_ctx, entity->shader_state, update_infos.slice(), curr_frame);
}

intern void bind_entity(
	Entity* entity,
	VulkanShader* shader,
	VulkanPipeline* pipeline,
	VulkanCmdBuffer cmd_buff,
	sz curr_frame
)
{
	shader->cmd_bind_entity_resources(cmd_buff, entity->shader_state, pipeline, curr_frame);
}

intern void begin_rendering(EngineContext* engine_ctx, VulkanCmdBuffer cmd_buffer)
{
	VulkanContext* vk_ctx = &engine_ctx->vk_ctx;
	VulkanSwapchain* swapchain = &vk_ctx->swapchain;
	VkImage curr_image         = swapchain->get_curr_image();
	VkImageView curr_view      = swapchain->get_curr_view();

	cmd_transition_layout_raw_image(
		curr_image,
		cmd_buffer,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		0,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		false
	);

	cmd_transition_layout_raw_image(
		vk_ctx->swapchain.depth_image.handle,
		cmd_buffer,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
		0,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		true
	);

	VkRenderingAttachmentInfo color_attachment_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = curr_view,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	};

	rg::mem_copy(color_attachment_info.clearValue.color.float32, &engine_ctx->renderer.clear_color, sizeof(f32) * 4);

	VkRenderingAttachmentInfo depth_attachment_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = vk_ctx->swapchain.depth_image.view,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.clearValue = { .depthStencil = CLEAR_DEPTH_VALUE },
	};

	VkRenderingInfo rendering_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = { .offset = OFFSET_START, .extent = engine_ctx->renderer.scissors.extent },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_info,
		.pDepthAttachment = &depth_attachment_info,
	};

	cmd_begin_rendering(cmd_buffer, &rendering_info);
}

intern void end_rendering(VulkanContext* vk_ctx, VulkanCmdBuffer cmd_buffer)
{
	cmd_end_rendering(cmd_buffer);
	VulkanSwapchain* swapchain = &vk_ctx->swapchain;

	cmd_transition_layout_raw_image(
		swapchain->get_curr_image(),
		cmd_buffer,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		0,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		false
	);

	cmd_transition_layout_raw_image(
		swapchain->depth_image.handle,
		cmd_buffer,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
		0,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		true
	);
}

// --- Global descriptors (view & projection matrices)

void Renderer::update_view_perspective()
{
	this->view = this->camera.look_at();
	this->proj = Mat4::perspective(rg::deg_to_rad(45.0f), this->aspect_ratio(), 0.1f, 100.0f);
	this->set_resize_scheduled(false);
	this->camera.dirty = false;
}

void Renderer::destroy()
{
	this->entities.destroy();
}

// --- Commands ---

intern void cmd_begin_rendering(VulkanCmdBuffer cmd_buff, VkRenderingInfo* rendering_info)
{
	vkCmdBeginRendering(cmd_buff.handle, rendering_info);
}

intern void cmd_end_rendering(VulkanCmdBuffer cmd_buff)
{
	vkCmdEndRendering(cmd_buff.handle);
}

intern void cmd_draw(VulkanCmdBuffer cmd_buff, GeometryView view)
{
	if (view.is_indexed())
	{
		vkCmdDrawIndexed(cmd_buff.handle, view.count(), 1, 0, 0, 0);
	}
	else
	{
		vkCmdDraw(cmd_buff.handle, view.count(), 1, 0, 0);
	}
}

intern void cmd_set_viewport_scissors(VulkanCmdBuffer cmd_buff, Renderer* renderer)
{
	vkCmdSetViewport(cmd_buff.handle, 0, 1, &renderer->viewport);
	vkCmdSetScissor(cmd_buff.handle, 0, 1, &renderer->scissors);
}

intern void cmd_push_mvp(VulkanCmdBuffer cmd_buff, VulkanPipeline* pipeline, const Mat4& model, const Mat4& RESTRICT view, const Mat4& RESTRICT proj)
{
	Mat4 mvp = model * view * proj;
	vkCmdPushConstants(cmd_buff.handle, pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mat4), &mvp);
}

// --- Event handlers ---

intern void init_event_handlers(Renderer* self, EventSystem* ev_sys)
{
	ev_sys->add_immediate_handler(MOUSE_MOVE, &handle_mouse_move, self);
	ev_sys->add_immediate_handler(KEY_DOWN, &handle_key_down_renderer, self);
	ev_sys->add_immediate_handler(KEY_DOWN, &handle_key_down_camera, self);
	// ev_sys->add_immediate_handler(KEY_DOWN, &handle_model_load, self);
	ev_sys->add_immediate_handler(MOUSE_WHEEL, &handle_mouse_wheel, self);
}

intern bool handle_mouse_move(EventContext ctx, void* listener)
{
	Renderer* renderer = (Renderer*)listener;
	f32 delta_x            = ctx.as_f32[0];
	f32 delta_y            = ctx.as_f32[1];
	renderer->camera.handle_mouse_move(delta_x, delta_y);
	return false;
}

intern bool handle_key_down_renderer(EventContext ctx, void* listener)
{
	Renderer* renderer = (Renderer*)listener;
	u32 key                 = ctx.as_u32[0];
	if (key == GLFW_KEY_ESCAPE)
	{
		renderer->set_exit_scheduled(true);
	}
	return false;
}

intern bool handle_key_down_camera(EventContext ctx, void* listener)
{
	Renderer* renderer    = (Renderer*)listener;
	u32 key                    = ctx.as_u32[0];
	if (!is_move_key(key)) return false;
	renderer->camera.handle_key_down(key, renderer->frame_clock.delta);
	return false;
}

intern bool handle_mouse_wheel(EventContext ctx, void* listener)
{
	Renderer* renderer = (Renderer*)listener;
	bool delta               = ctx.as_b8[0];
	renderer->camera.handle_mouse_wheel(delta);
	return false;
}

// Geometry.

intern Array<Vertex, CUBE_VERT_COUNT> get_cube_vertices()
{
    Array<Vertex, CUBE_VERT_COUNT> cube_vertices = {
		// front
		{ POS_TLN, NORMAL_FORWARD, TEX_COORD_TL },
		{ POS_BLN, NORMAL_FORWARD, TEX_COORD_BL },
		{ POS_BRN, NORMAL_FORWARD, TEX_COORD_BR },
		{ POS_TRN, NORMAL_FORWARD, TEX_COORD_TR },

		// back
		{ POS_TLF, NORMAL_BACKWARD, TEX_COORD_TL },
		{ POS_BLF, NORMAL_BACKWARD, TEX_COORD_BL },
		{ POS_BRF, NORMAL_BACKWARD, TEX_COORD_BR },
		{ POS_TRF, NORMAL_BACKWARD, TEX_COORD_TR },

		// top
		{ POS_TLN, NORMAL_UP, TEX_COORD_TL },
		{ POS_TRN, NORMAL_UP, TEX_COORD_TR },
		{ POS_TLF, NORMAL_UP, TEX_COORD_BL },
		{ POS_TRF, NORMAL_UP, TEX_COORD_BR },

		// bottom
		{ POS_BLN, NORMAL_DOWN, TEX_COORD_TL },
		{ POS_BRN, NORMAL_DOWN, TEX_COORD_TR },
		{ POS_BLF, NORMAL_DOWN, TEX_COORD_BL },
		{ POS_BRF, NORMAL_DOWN, TEX_COORD_BR },

		// right
		{ POS_TRN, NORMAL_RIGHT, TEX_COORD_TL },
		{ POS_BRN, NORMAL_RIGHT, TEX_COORD_BL },
		{ POS_TRF, NORMAL_RIGHT, TEX_COORD_TR },
		{ POS_BRF, NORMAL_RIGHT, TEX_COORD_BR },

		// left
		{ POS_TLN, NORMAL_LEFT, TEX_COORD_TL },
		{ POS_BLN, NORMAL_LEFT, TEX_COORD_BL },
		{ POS_TLF, NORMAL_LEFT, TEX_COORD_TR },
		{ POS_BLF, NORMAL_LEFT, TEX_COORD_BR },
	};

	return cube_vertices;
}

// This will temp allocate index buffer.
intern IndexBuffer get_cube_indices(IndexStride stride)
{	
 	static Array<u32, CUBE_INDEX_COUNT> indices = {
		0, 1, 2,        0, 2, 3,
		4, 6, 5,        4, 7, 6,
		8, 9, 10,       9, 11, 10,
		12, 14, 13,     13, 14, 15,
		16, 17, 18,     17, 19, 18,
		20, 22, 21,     21, 22, 23
	};

	// TODO: fix
	// Arena* talloc = get_temp_allocator();
	const sz CUBE_INDEX_BYTE_SIZE = CUBE_INDEX_COUNT * (sz)stride;
	// Slice<u8> indices_as_bytes = { (u8*)allocator_allocate(talloc, CUBE_INDEX_BYTE_SIZE), CUBE_INDEX_BYTE_SIZE };
	Slice<u8> indices_as_bytes = { (u8*)indices.data, CUBE_INDEX_BYTE_SIZE };
	IndexBuffer idx_buff = IndexBuffer::create(indices_as_bytes, stride);
	return idx_buff;
}

// TODO: move into gltf.cpp
intern GltfSampler get_default_gltf_sampler()
{
	return { .mag_filter = GltfSamplerMagFilter::LINEAR, .min_filter = GltfSamplerMinFilter::LINEAR_MIPMAP_LINEAR, .wrap_s = GltfSamplerWrapKind::REPEAT, .wrap_t = GltfSamplerWrapKind::REPEAT, };
}

// intern bool handle_model_load(EventContext ctx, void* listener)
// {
// 	Renderer* renderer = (Renderer*)listener;
// 	u32 key                 = ctx.as_u32[0];

// 	if (key == MODEL_LOAD_KEY)
// 	{
// 		Path[] paths     = f_arena.alloc_array(Path, 1);
// 		Path? model_path = path::new(f_arena, MODELS_PATH +++ "scifi_helmet/SciFiHelmet.gltf");
// 		if (catch model_path) return false;
// 		paths[0] = model_path;

// 		LoadModelsTaskData task_data = { .paths = paths, .engine_ctx->= renderer.engine_ctx-> };

// 		BackgroundTask task = { .kind = LOAD_MODELS, .payload = { .load_models = task_data }, };
// 		renderer_add_background_task(renderer, task);
// 	}

// 	return false;
// }

// --- Background task system ---

// struct BackgroundTask
// {
// 	BackgroundTaskKind kind;
// 	BackgroundTaskPayload payload;
// }

// enum BackgroundTaskKind
// {
// 	LOAD_MODELS
// }

// union BackgroundTaskPayload
// {
// 	BackgroundTaskLoadModels load_models;
// }

// struct BackgroundTaskLoadModels
// {
// 	Path[] paths;
// }

// intern void renderer_add_background_task(Renderer* self, BackgroundTask task)
// {
// 	self.background_tasks.push(task);
// }

// intern void renderer_process_background_tasks(void* arg)
// {
// 	Renderer* self = arg;
// 	BackgroundTask task @noinit;

// 	while (!self.background_tasks.is_empty())
// 	{
// 		task = self.background_tasks.pop_first_ensure();
// 		switch (task.kind)
// 		{
// 			// NOTE: TEMP
// 			case LOAD_MODELS: renderer_process_background_load_models_task(self, task.payload.load_models);
// 			// case LOAD_MODELS: renderer_process_background_load_models_task_temp(self, task);
// 			default: UNREACHABLE("TODO: execute background task with kind: %s", task.kind);
// 		}
// 	}

// 	// TODO + THINK: wait for execution here?
// }

// intern void renderer_process_background_load_models_task(Renderer* self, BackgroundTaskLoadModels payload)
// {
// 	App* engine_ctx->                 = self.engine_ctx->
// 	MainThreadPool* tpool         = &engine_ctx->tpool;
// 	LoadModelsTaskData[] task_data = f_arena.alloc_array(LoadModelsTaskData, payload.paths.len);
// 	ThreadTask[] thread_tasks     = f_arena.alloc_array(ThreadTask, payload.paths.len);

// 	foreach (i, &task : thread_tasks)
// 	{
// 		task_data[i].path = payload.paths[i];
// 		task_data[i].engine_ctx->= self.engine_ctx->
// 		task.func = &gltf::load_model_task;
// 		task.arg = &task_data[i];
// 	}
	
// 	// Submit, without wait.
// 	tpool.submit_task_many(thread_tasks);
// }

// <*
//  Loads resources at application startup.
// *>
// intern void renderer_perform_startup_load(Renderer* renderer)
// {
// 	GeometryView cube_geometry = renderer.elem_buff.append_geometry_indexed(
// 		engine::get_cube_vertices()[..],
// 		engine::get_cube_indices(),
// 	);

// 	GltfSampler default_sampler = gltf::get_default_sampler();

// 	// TODO: not effective, we're loading textures 1 by 1, creating threads for that.
// 	// We should load them in 1 batch.
// 	// TextureCreateConfig[*] tex_configs = {
// 	// 	{ path::new(p_arena, engine::TEXTURES_PATH +++ "grass.jpg")!!, default_sampler, TEX_INDEX_INVALID, DIFFUSE },
// 	// 	{ path::new(p_arena, engine::TEXTURES_PATH +++ "soil.jpg")!!, default_sampler, TEX_INDEX_INVALID, DIFFUSE },
// 	// 	{ path::new(p_arena, engine::TEXTURES_PATH +++ "sand.jpg")!!, default_sampler, TEX_INDEX_INVALID, DIFFUSE },
// 	// 	{ path::new(p_arena, engine::TEXTURES_PATH +++ "metal.jpg")!!, default_sampler, TEX_INDEX_INVALID, DIFFUSE },
// 	// 	{ path::new(p_arena, engine::TEXTURES_PATH +++ "water.jpg")!!, default_sampler, TEX_INDEX_INVALID, DIFFUSE },
// 	// 	{ path::new(p_arena, engine::TEXTURES_PATH +++ "stones.jpg")!!, default_sampler, TEX_INDEX_INVALID, DIFFUSE },
// 	// };

// 	App* engine_ctx->    = renderer.engine_ctx->
// 	FrameData* frame = engine_ctx->vk_ctx.get_curr_frame_data();

// 	cmd_buff_begin_recording_many(frame.transfer_cmd_buffs[..], frame.transfer_cmd_buff_states[..],);

// 	t_arena.@temp_scope()
// 	{
// 		// engine_ctx->tex_sys.load_textures_cpu_and_transfer_to_gpu(
// 		// 	engine_ctx->
// 		// 	tex_configs[..],
// 		// 	frame.transfer_cmd_buffs[..],
// 		// 	frame.transfer_cmd_buff_states[..]
// 		// );

// 		// renderer.add_entity(cube_geometry, { tex_configs[0] }, entity::transform_tagged_create({ 0, 2, 0 }),);
// 		// renderer.add_entity(cube_geometry, { tex_configs[1] }, entity::transform_tagged_create({ 2, 3.5, -3 }),);
// 		// renderer.add_entity(cube_geometry, { tex_configs[2] }, entity::transform_tagged_create({ 0.7, -0.1, 0 }),);
// 		// renderer.add_entity(cube_geometry, { tex_configs[3] }, entity::transform_tagged_create({ -0.1, 0.8, 0 }),);
// 		// renderer.add_entity(cube_geometry, { tex_configs[4] }, entity::transform_tagged_create({ 0.4, 0.4, -1.0 }),);
// 		// renderer.add_entity(cube_geometry, { tex_configs[5] }, entity::transform_tagged_create({ 0.7, 0.7, 1.0 }),);
// 		// renderer.add_entity(cube_geometry, { tex_configs[2] }, entity::transform_tagged_create({ 0.7, 0.6, -3.0 }),);

// 		// NOTE: TEMP -->
// 		Path[] paths    = f_arena.alloc_array(Path, 1);
// 		Path model_path = path::new(f_arena, MODELS_PATH +++ "scifi_helmet/SciFiHelmet.gltf")!!;
// 		paths[0] = model_path;
// 		LoadModelsTaskData task_data = { .paths = paths, .engine_ctx->= renderer.engine_ctx-> };
// 		BackgroundTask task          = { .kind = LOAD_MODELS, .payload = { .load_models = task_data }, };
// 		// TODO: this is not loading anything.
// 		renderer_process_background_load_models_task(renderer, task);
// 		// renderer_process_background_load_models_task_temp(renderer, task);
// 		// NOTE: TEMP <--

// 		cmd_buff_end_submit_reset_many(
// 			frame.transfer_cmd_buffs[..],
// 			frame.transfer_cmd_buff_states[..],
// 			&engine_ctx->vk_ctx,
// 			engine_ctx->vk_ctx.dev.transfer_queue,
// 			true
// 		);
// 	};
// }

} // rg
