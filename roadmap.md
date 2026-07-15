# Real Engine Development Roadmap

## What You've Built (Foundation Layer) ✅

| System | Status | Notes |
|---|---|---|
| Custom types, allocators, math | ✅ Done | Vmem, Arena, Pool, TLSF, Heap |
| Containers (DArray, FArray, HashMap, HashSet, RingBuffer) | ✅ Done | SoA layout, explicit allocators |
| Strings (DString, FString, StrView, UTF-8) | ✅ Done | |
| Threading (Thread, Mutex, CV, Semaphore, ThreadPool) | ✅ Done | Lock-free variant experimented |
| Atomics, Bits/BitSets | ✅ Done | |
| SIMD Math (Vec2/3/4, Mat4) | ✅ Done | AVX-512 Mat4 mul |
| I/O & File System (read, write, mmap) | ✅ Done | |
| Conversions, RNG, Hashing, Defer | ✅ Done | |

---

## Phase 1: Platform & Windowing

**Goal:** Get a window on screen with an event loop.

| Task | Details |
|---|---|
| **Platform abstraction layer** | Formalize your existing `#ifdef` pattern into a `platform.hpp` that wraps: windowing, input, timing, dynamic library loading |
| **Clock / Timer** | High-resolution clock using `clock_gettime` / `QueryPerformanceCounter` — you noted this in `todo.md` as incomplete |
| **Window creation** | Use raw OS APIs (X11/Wayland on POSIX, Win32 on Windows) or SDL3 if you're willing to bend the no-deps rule. SDL3 is C, no C++ features |
| **Input system** | Keyboard, mouse, gamepad state tracking via platform events |
| **Event loop** | Poll/dispatch event system for window resize, close, input, etc. |

**Dependencies:** None — builds directly on your platform abstractions.

---

## Phase 2: Vulkan Initialization

**Goal:** Create a Vulkan instance, surface, physical/logical device, swapchain.

| Task | Details |
|---|---|
| **Vulkan instance & surface** | Load Vulkan dynamically (`dlopen`/`LoadLibrary`), create instance, enumerate extensions, create surface from window |
| **Physical device selection** | Enumerate GPUs, score them, pick best (dedicated > integrated, check surface format/present queue support) |
| **Logical device & queues** | Create device with graphics + present queue families, enable required extensions (swapchain) |
| **Swapchain** | Create, acquire, present — handle resize with `VK_SUBOPTIMAL_KHR` / `VK_ERROR_OUT_OF_DATE_KHR` |
| **Vulkan memory allocator** | Decide: use your existing `VmemAllocator` as backing, or integrate VMA (Vulkan Memory Allocator — C library, ~12k lines, no deps). VMA handles defragmentation, staging buffers, and memory pools for GPU memory |
| **Command buffers & pools** | Per-frame command buffer allocation, reset, and submission |

**Key design decision:** Your allocator philosophy (explicit, no hidden allocation) maps well to Vulkan. The question is whether to write your own Vulkan memory management (complex, error-prone) or use VMA as a "third-party like TLSF."

---

## Phase 3: Render Pipeline

**Goal:** Draw a triangle on screen, then build up to a render graph.

| Task | Details |
|---|---|
| **Basic pipeline** | Vertex/input assembly, render pass, framebuffer, pipeline layout, descriptor sets |
| **Shader loading & compilation** | Load SPIR-V from disk (use your file I/O). Optionally integrate `shaderc` or `glslang` for GLSL→SPIR-V at build time |
| **Swapchain render loop** | Acquire → record commands → submit → present, with proper synchronization (semaphores, fences) |
| **Resource management** | Handle for GPU resources (buffers, images, pipelines) — pool allocator works well here |
| **Render graph** | DAG of render passes with automatic dependency tracking, resource barriers, and pass ordering. This is the backbone for deferred rendering, post-processing, shadows, etc. |
| **ImGui integration** | Dear ImGui (C++, but no STL by default, uses `new` minimally — could be adapted). Essential for debug UI, editor tooling, and prototyping |

---

## Phase 4: Entity Component System (ECS)

**Goal:** Data-oriented entity management for scenes.

| Task | Details |
|---|---|
| **Sparse set or archetype ECS** | **Sparse set:** simpler, good for small component counts. **Archetype:** better cache locality for iteration, more complex. Your SoA preference in HashMap suggests archetype may appeal |
| **Component storage** | Each component type gets its own dense/sparse array or archetype table. Use your `DArray` with explicit allocator |
| **Entity lifecycle** | Create, destroy, recycle IDs (use a free-list or generation counter) |
| **System execution** | Iterate entities matching component signatures, run system logic. Can integrate with thread pool for parallel system execution |
| **Tags & relationships** | Lightweight way to mark entities or create parent-child, sibling relationships without full components |

**Design alignment:** ECS is inherently data-oriented and allocator-friendly — no hidden allocations, cache-friendly iteration, explicit lifetime. Fits your philosophy perfectly.

---

## Phase 5: Scene & Transform System

**Goal:** Hierarchical transforms, scene tree, serialization.

| Task | Details |
|---|---|
| **Transform component** | Local + world matrix, dirty flag for lazy world-matrix recomputation. Use your existing `Mat4` |
| **Scene graph / hierarchy** | Parent-child relationships between entities. Traverse for world-space queries |
| **Camera** | Perspective/orthographic projection, view matrix from transform, frustum planes for culling |
| **Serialization** | Binary format for scenes (your `FArray`/`DArray` + file I/O are ready). JSON or custom text format for human editing |
| **Asset references** | Handle-based references to meshes, textures, materials — decouple asset lifetime from entity lifetime |

---

## Phase 6: Mesh & Material Pipeline

**Goal:** Load 3D models, assign materials, prepare for rendering.

| Task | Details |
|---|---|
| **Vertex format** | Define vertex layout (position, normal, UV, tangent, etc.) — use your `BitInt` for vertex attribute flags |
| **Mesh loading** | Load glTF 2.0 (via `cgltf` — C library, ~1500 lines, no deps). Parse into your `DArray`/vertex buffers |
| **Material system** | Material = combination of shader + uniform data + textures. Use handle-based approach with a material pool |
| **Texture loading** | Stb_image (C, header-only) for PNG/JPG → GPU image upload. Or `ktx` for KTX2/Basis |
| **Mesh rendering** | Submit vertex/index buffers to render graph, bind material/pipeline, draw |

---

## Phase 7: Physics & Collision

**Goal:** Rigid body simulation and collision detection.

| Task | Details |
|---|---|
| **Broad phase** | Spatial hashing or AABB tree for fast overlap detection. Use your `HashMap` for spatial grid |
| **Narrow phase** | SAT (Separating Axis Theorem) for convex polygons, GJK for arbitrary convex shapes |
| **Collision response** | Impulse-based resolution, restitution, friction |
| **Integration** | Semi-implicit Euler or Verlet integration for position/velocity updates |
| **Rigid body component** | Mass, inertia tensor, velocity, angular velocity — as an ECS component |
| **Physics world** | Step function called each frame, iterates broad→narrow→resolve→integrate |

**Note:** Full physics is a rabbit hole. Start with AABB collisions + simple impulse response, iterate.

---

## Phase 8: Audio

**Goal:** 3D positional audio, mixing, streaming.

| Task | Details |
|---|---|
| **Audio backend** | Platform audio APIs directly (ALSA/CoreAudio/WASAPI) or a thin C wrapper. Miniaudio is a single-file C library (~14k lines, no deps) |
| **Sound buffers** | Load WAV/OGG into your allocators, submit to audio device |
| **3D spatialization** | Distance attenuation, Doppler, panning based on listener/source positions |
| **Audio mixer** | Mix multiple sources, apply volume/panning/EQ per channel |
| **Streaming** | Stream large audio files from disk in chunks to avoid loading entire file into memory |

---

## Phase 9: Editor & Tooling

**Goal:** In-engine editor for building scenes, debugging, profiling.

| Task | Details |
|---|---|
| **Entity inspector** | Browse entities, view/edit components via ImGui |
| **Scene hierarchy panel** | Tree view of entity parent-child relationships |
| **Viewport** | Render scene to an offscreen FBO, display in ImGui panel with mouse interaction for camera control |
| **Asset browser** | Browse loaded assets, drag-drop into scene |
| **Performance profiler** | Frame time breakdown, system timing, memory usage display |
| **Console / log viewer** | Redirect your `LOG_*` macros to an ImGui panel |

---

## Phase 10: Polish & Optimization

| Task | Details |
|---|---|
| **Frustum culling** | Skip rendering entities outside camera frustum |
| **LOD system** | Distance-based mesh/texture LOD switching |
| **Shadow mapping** | Cascaded shadow maps for directional lights, point light shadow maps |
| **Deferred rendering** | G-buffer pass + lighting pass via render graph |
| **Instanced rendering** | Batch same-mesh draws into instanced draw calls |
| **Memory profiling** | Track allocations per frame, detect leaks, visualize allocator usage |
| **Hot reload** | Dynamic library loading for game logic, shader hot-reload |

---

## Recommended Build Order

```
Phase 1  (Platform)    ──→  Phase 2 (Vulkan)  ──→  Phase 3 (Render)
                                                           │
Phase 4  (ECS)  ──→  Phase 5 (Scene)  ──→  Phase 6 (Mesh/Mat)
                                                │
                              ┌──────────────────┤
                              │                  │
                         Phase 7             Phase 8
                        (Physics)           (Audio)
                              │                  │
                              └──────┬───────────┘
                                     │
                              Phase 9 (Editor)
                                     │
                              Phase 10 (Polish)
```

**Phases 1-3** are the critical path — nothing visual happens without them. **Phase 4 (ECS)** can be started in parallel with Phase 2/3 since it's purely logical. **Phases 7-8** (physics/audio) are independent and can be deferred.

---

## Immediate Next Steps

1. **Clock/timer** — finish your `todo.md` item, needed for all frame timing
2. **Window creation** — decide SDL3 vs raw OS APIs (tradeoff: portability vs philosophy purity)
3. **Vulkan bootstrap** — fill those empty stubs, get a swapchain presenting
